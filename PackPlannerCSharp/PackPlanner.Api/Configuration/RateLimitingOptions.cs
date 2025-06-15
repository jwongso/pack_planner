namespace PackPlanner.Api.Configuration;

/// <summary>
/// Configuration options for the rate limiting middleware
/// Provides comprehensive settings for different endpoints and global behavior
/// </summary>
public class RateLimitingOptions
{
    /// <summary>
    /// Configuration section name in appsettings.json
    /// </summary>
    public const string SectionName = "RateLimiting";

    /// <summary>
    /// Global rate limiting settings that apply across all endpoints
    /// </summary>
    public GlobalRateLimitSettings GlobalSettings { get; set; } = new();

    /// <summary>
    /// Endpoint-specific rate limiting policies
    /// </summary>
    public EndpointRateLimitPolicies EndpointPolicies { get; set; } = new();
}

/// <summary>
/// Global settings for rate limiting behavior
/// </summary>
public class GlobalRateLimitSettings
{
    /// <summary>
    /// Master switch to enable/disable rate limiting entirely
    /// </summary>
    public bool EnableRateLimiting { get; set; } = true;

    /// <summary>
    /// Default time window size in minutes for rate limiting
    /// Used when no specific policy is defined for an endpoint
    /// </summary>
    public int DefaultWindowSizeMinutes { get; set; } = 1;

    /// <summary>
    /// Default maximum number of requests allowed per window
    /// Used when no specific policy is defined for an endpoint
    /// </summary>
    public int DefaultRequestLimit { get; set; } = 100;

    /// <summary>
    /// Whether to enable IP address whitelisting
    /// Whitelisted IPs bypass all rate limiting
    /// </summary>
    public bool EnableIpWhitelist { get; set; } = true;

    /// <summary>
    /// List of IP addresses that bypass rate limiting
    /// Typically includes localhost and trusted monitoring systems
    /// </summary>
    public List<string> WhitelistedIPs { get; set; } = new() { "127.0.0.1", "::1" };
}

/// <summary>
/// Container for all endpoint-specific rate limiting policies
/// Each property represents a different category of endpoints
/// </summary>
public class EndpointRateLimitPolicies
{
    /// <summary>
    /// Rate limiting policy for the main pack planning endpoint
    /// More restrictive due to computational cost
    /// </summary>
    public RateLimitPolicy PackPlanning { get; set; } = new()
    {
        WindowSizeMinutes = 1,
        RequestLimit = 10,
        Description = "Main pack planning endpoint - limited due to computational cost"
    };

    /// <summary>
    /// Rate limiting policy for async pack planning endpoint
    /// Most restrictive due to resource usage and job creation
    /// </summary>
    public RateLimitPolicy AsyncPackPlanning { get; set; } = new()
    {
        WindowSizeMinutes = 1,
        RequestLimit = 5,
        Description = "Async pack planning - more restrictive due to resource usage"
    };

    /// <summary>
    /// Rate limiting policy for health check endpoints
    /// More permissive to allow monitoring systems to function
    /// </summary>
    public RateLimitPolicy HealthCheck { get; set; } = new()
    {
        WindowSizeMinutes = 1,
        RequestLimit = 60,
        Description = "Health check endpoint - higher limit for monitoring"
    };

    /// <summary>
    /// Rate limiting policy for informational endpoints
    /// Moderate limits for endpoints that return static information
    /// </summary>
    public RateLimitPolicy Information { get; set; } = new()
    {
        WindowSizeMinutes = 1,
        RequestLimit = 30,
        Description = "Information endpoints (strategies, sort-orders, version)"
    };

    /// <summary>
    /// Rate limiting policy for job management endpoints
    /// Moderate limits for job status and result retrieval
    /// </summary>
    public RateLimitPolicy JobManagement { get; set; } = new()
    {
        WindowSizeMinutes = 1,
        RequestLimit = 20,
        Description = "Job status and result endpoints"
    };
}

/// <summary>
/// Defines rate limiting parameters for a specific endpoint or group of endpoints
/// </summary>
public class RateLimitPolicy
{
    /// <summary>
    /// Time window size in minutes for rate limiting
    /// Requests are counted within this sliding window
    /// </summary>
    public int WindowSizeMinutes { get; set; } = 1;

    /// <summary>
    /// Maximum number of requests allowed within the time window
    /// </summary>
    public int RequestLimit { get; set; } = 100;

    /// <summary>
    /// Human-readable description of this policy
    /// Used for documentation and logging purposes
    /// </summary>
    public string Description { get; set; } = string.Empty;
}
