using System.Collections.Concurrent;
using System.Net;
using System.Text.Json;
using System.Threading;
using PackPlanner.Api.Configuration;

namespace PackPlanner.Api.Middleware;

/// <summary>
/// Rate limiting middleware that provides configurable request throttling
/// to prevent DoS attacks and API misuse
/// </summary>
public class RateLimitingMiddleware
{
    private readonly RequestDelegate _next;
    private readonly ILogger<RateLimitingMiddleware> _logger;
    private readonly RateLimitingOptions _options;

    /// <summary>
    /// Thread-safe dictionary to store client request counts
    /// Key: ClientId (IP + Endpoint), Value: Request tracking data
    /// </summary>
    private static readonly ConcurrentDictionary<string, ClientRequestInfo> _clientRequests = new();

    /// <summary>
    /// Timer for periodic cleanup of expired request tracking data
    /// </summary>
    private static readonly System.Threading.Timer _cleanupTimer = new System.Threading.Timer(
        CleanupExpiredEntries,
        null,
        TimeSpan.FromMinutes(5),
        TimeSpan.FromMinutes(5)
    );

    /// <summary>
    /// Initializes the rate limiting middleware
    /// </summary>
    /// <param name="next">Next middleware in the pipeline</param>
    /// <param name="logger">Logger for rate limiting events</param>
    /// <param name="options">Rate limiting configuration options</param>
    public RateLimitingMiddleware(RequestDelegate next, ILogger<RateLimitingMiddleware> logger, RateLimitingOptions options)
    {
        _next = next;
        _logger = logger;
        _options = options;
    }

    /// <summary>
    /// Main middleware execution method that checks rate limits before allowing requests
    /// </summary>
    /// <param name="context">HTTP context for the current request</param>
    public async Task InvokeAsync(HttpContext context)
    {
        // Skip rate limiting if disabled globally
        if (!_options.GlobalSettings.EnableRateLimiting)
        {
            await _next(context);
            return;
        }

        // Get client identifier (IP address)
        var clientId = GetClientIdentifier(context);

        // Check if client is whitelisted
        if (IsWhitelisted(clientId))
        {
            _logger.LogDebug("Request from whitelisted IP {ClientId} allowed without rate limiting", clientId);
            await _next(context);
            return;
        }

        // Determine the appropriate rate limit policy for this endpoint
        var policy = GetRateLimitPolicy(context.Request.Path);
        var clientKey = $"{clientId}:{context.Request.Path}";

        // Check if request should be rate limited
        var rateLimitResult = CheckRateLimit(clientKey, policy);

        if (rateLimitResult.IsAllowed)
        {
            // Add rate limit headers to response
            AddRateLimitHeaders(context, rateLimitResult);

            _logger.LogDebug("Request from {ClientId} to {Path} allowed. Remaining: {Remaining}",
                clientId, context.Request.Path, rateLimitResult.RemainingRequests);

            await _next(context);
        }
        else
        {
            // Request exceeded rate limit - return 429 Too Many Requests
            await HandleRateLimitExceeded(context, rateLimitResult, clientId);
        }
    }

    /// <summary>
    /// Extracts client identifier from the HTTP context
    /// Prioritizes X-Forwarded-For header for proxy scenarios
    /// </summary>
    /// <param name="context">HTTP context</param>
    /// <returns>Client IP address as string</returns>
    private static string GetClientIdentifier(HttpContext context)
    {
        // Check for X-Forwarded-For header (common in proxy/load balancer scenarios)
        var forwardedFor = context.Request.Headers["X-Forwarded-For"].FirstOrDefault();
        if (!string.IsNullOrEmpty(forwardedFor))
        {
            // Take the first IP in the chain (original client)
            var firstIp = forwardedFor.Split(',')[0].Trim();
            return firstIp;
        }

        // Check for X-Real-IP header (alternative proxy header)
        var realIp = context.Request.Headers["X-Real-IP"].FirstOrDefault();
        if (!string.IsNullOrEmpty(realIp))
        {
            return realIp;
        }

        // Fall back to connection remote IP
        return context.Connection.RemoteIpAddress?.ToString() ?? "unknown";
    }

    /// <summary>
    /// Checks if a client IP is in the whitelist
    /// </summary>
    /// <param name="clientId">Client IP address</param>
    /// <returns>True if whitelisted, false otherwise</returns>
    private bool IsWhitelisted(string clientId)
    {
        if (!_options.GlobalSettings.EnableIpWhitelist ||
            _options.GlobalSettings.WhitelistedIPs == null)
        {
            return false;
        }

        return _options.GlobalSettings.WhitelistedIPs.Contains(clientId);
    }

    /// <summary>
    /// Determines the appropriate rate limit policy based on the request path
    /// </summary>
    /// <param name="path">Request path</param>
    /// <returns>Rate limit policy configuration</returns>
    private RateLimitPolicy GetRateLimitPolicy(PathString path)
    {
        var pathStr = path.Value?.ToLowerInvariant() ?? "";

        // Match specific endpoint patterns to policies
        var policy = pathStr switch
        {
            "/api/pack" => _options.EndpointPolicies.PackPlanning,
            "/api/pack/async" => _options.EndpointPolicies.AsyncPackPlanning,
            "/api/health" or "/health" => _options.EndpointPolicies.HealthCheck,
            var p when p.StartsWith("/api/pack/status/") || p.StartsWith("/api/pack/result/") || p.StartsWith("/api/pack/job/")
                => _options.EndpointPolicies.JobManagement,
            var p when p.StartsWith("/api/strategies") || p.StartsWith("/api/sort-orders") || p.StartsWith("/api/version")
                => _options.EndpointPolicies.Information,
            _ => new RateLimitPolicy
            {
                WindowSizeMinutes = _options.GlobalSettings.DefaultWindowSizeMinutes,
                RequestLimit = _options.GlobalSettings.DefaultRequestLimit,
                Description = "Default policy"
            }
        };

        return policy;
    }

    /// <summary>
    /// Checks if a request should be allowed based on rate limiting rules
    /// </summary>
    /// <param name="clientKey">Unique key for client + endpoint combination</param>
    /// <param name="policy">Rate limit policy to apply</param>
    /// <returns>Rate limit check result</returns>
    private static RateLimitResult CheckRateLimit(string clientKey, RateLimitPolicy policy)
    {
        var now = DateTime.UtcNow;
        var windowStart = now.AddMinutes(-policy.WindowSizeMinutes);

        // Get or create client request info
        var clientInfo = _clientRequests.AddOrUpdate(clientKey,
            // Factory for new entry
            new ClientRequestInfo
            {
                RequestTimes = new List<DateTime> { now },
                WindowStart = windowStart
            },
            // Update function for existing entry
            (key, existing) =>
            {
                lock (existing)
                {
                    // Remove requests outside the current window
                    existing.RequestTimes.RemoveAll(time => time < windowStart);

                    // Add current request
                    existing.RequestTimes.Add(now);
                    existing.WindowStart = windowStart;

                    return existing;
                }
            });

        lock (clientInfo)
        {
            var requestCount = clientInfo.RequestTimes.Count;
            var isAllowed = requestCount <= policy.RequestLimit;
            var remainingRequests = Math.Max(0, policy.RequestLimit - requestCount + 1);

            // Calculate when the window resets (when the oldest request expires)
            var oldestRequest = clientInfo.RequestTimes.Min();
            var resetTime = oldestRequest.AddMinutes(policy.WindowSizeMinutes);

            return new RateLimitResult
            {
                IsAllowed = isAllowed,
                RequestCount = requestCount,
                RequestLimit = policy.RequestLimit,
                RemainingRequests = remainingRequests,
                WindowSizeMinutes = policy.WindowSizeMinutes,
                ResetTime = resetTime
            };
        }
    }

    /// <summary>
    /// Adds rate limiting information to response headers
    /// </summary>
    /// <param name="context">HTTP context</param>
    /// <param name="result">Rate limit check result</param>
    private static void AddRateLimitHeaders(HttpContext context, RateLimitResult result)
    {
        context.Response.Headers["X-RateLimit-Limit"] = result.RequestLimit.ToString();
        context.Response.Headers["X-RateLimit-Remaining"] = result.RemainingRequests.ToString();
        context.Response.Headers["X-RateLimit-Reset"] = ((DateTimeOffset)result.ResetTime).ToUnixTimeSeconds().ToString();
        context.Response.Headers["X-RateLimit-Window"] = $"{result.WindowSizeMinutes}m";
    }

    /// <summary>
    /// Handles requests that exceed rate limits by returning 429 status
    /// </summary>
    /// <param name="context">HTTP context</param>
    /// <param name="result">Rate limit check result</param>
    /// <param name="clientId">Client identifier for logging</param>
    private async Task HandleRateLimitExceeded(HttpContext context, RateLimitResult result, string clientId)
    {
        // Log rate limit violation
        _logger.LogWarning("Rate limit exceeded for client {ClientId} on path {Path}. " +
                          "Requests: {RequestCount}/{RequestLimit}, Window: {WindowSize}m",
            clientId, context.Request.Path, result.RequestCount, result.RequestLimit, result.WindowSizeMinutes);

        // Set response status and headers
        context.Response.StatusCode = (int)HttpStatusCode.TooManyRequests;
        context.Response.ContentType = "application/json";

        AddRateLimitHeaders(context, result);

        // Add Retry-After header
        var retryAfterSeconds = (int)(result.ResetTime - DateTime.UtcNow).TotalSeconds;
        context.Response.Headers["Retry-After"] = Math.Max(1, retryAfterSeconds).ToString();

        // Create error response
        var errorResponse = new
        {
            error = "Rate limit exceeded",
            details = $"Too many requests. Limit: {result.RequestLimit} requests per {result.WindowSizeMinutes} minute(s)",
            retryAfter = retryAfterSeconds,
            timestamp = DateTime.UtcNow
        };

        var jsonResponse = JsonSerializer.Serialize(errorResponse, new JsonSerializerOptions
        {
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
            WriteIndented = true
        });

        await context.Response.WriteAsync(jsonResponse);
    }

    /// <summary>
    /// Periodic cleanup method to remove expired request tracking data
    /// Prevents memory leaks from accumulating client data
    /// </summary>
    /// <param name="state">Timer state (unused)</param>
    private static void CleanupExpiredEntries(object? state)
    {
        var cutoffTime = DateTime.UtcNow.AddHours(-1); // Remove data older than 1 hour
        var keysToRemove = new List<string>();

        foreach (var kvp in _clientRequests)
        {
            lock (kvp.Value)
            {
                // Remove old request times
                kvp.Value.RequestTimes.RemoveAll(time => time < cutoffTime);

                // Mark for removal if no recent requests
                if (kvp.Value.RequestTimes.Count == 0)
                {
                    keysToRemove.Add(kvp.Key);
                }
            }
        }

        // Remove empty entries
        foreach (var key in keysToRemove)
        {
            _clientRequests.TryRemove(key, out _);
        }
    }
}

/// <summary>
/// Stores request tracking information for a specific client
/// </summary>
internal class ClientRequestInfo
{
    /// <summary>
    /// List of request timestamps within the current window
    /// </summary>
    public List<DateTime> RequestTimes { get; set; } = new();

    /// <summary>
    /// Start time of the current rate limiting window
    /// </summary>
    public DateTime WindowStart { get; set; }
}

/// <summary>
/// Result of a rate limit check operation
/// </summary>
internal class RateLimitResult
{
    /// <summary>
    /// Whether the request should be allowed
    /// </summary>
    public bool IsAllowed { get; set; }

    /// <summary>
    /// Current number of requests in the window
    /// </summary>
    public int RequestCount { get; set; }

    /// <summary>
    /// Maximum allowed requests in the window
    /// </summary>
    public int RequestLimit { get; set; }

    /// <summary>
    /// Number of requests remaining in the current window
    /// </summary>
    public int RemainingRequests { get; set; }

    /// <summary>
    /// Size of the rate limiting window in minutes
    /// </summary>
    public int WindowSizeMinutes { get; set; }

    /// <summary>
    /// When the current rate limiting window resets
    /// </summary>
    public DateTime ResetTime { get; set; }
}
