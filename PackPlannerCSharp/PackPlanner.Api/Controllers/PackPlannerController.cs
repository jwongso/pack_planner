using Microsoft.AspNetCore.Mvc;
using PackPlanner.Api.Models;
using System.Diagnostics;
using System.Collections.Concurrent;

namespace PackPlanner.Api.Controllers;

/// <summary>
/// Main controller for pack planning operations
/// Provides endpoints for pack planning, health checks, and system information
/// </summary>
[ApiController]
[Route("api")]
[Produces("application/json")]
public class PackPlannerController : ControllerBase
{
    private readonly ILogger<PackPlannerController> _logger;
    private readonly PackPlanner _packPlanner;
    private static readonly ConcurrentDictionary<string, AsyncPackJob> _jobs = new();

    /// <summary>
    /// Initializes a new instance of the PackPlannerController
    /// </summary>
    /// <param name="logger">Logger instance for request logging</param>
    public PackPlannerController(ILogger<PackPlannerController> logger)
    {
        _logger = logger;
        _packPlanner = new PackPlanner();
    }

    /// <summary>
    /// Main pack planning endpoint
    /// Accepts a list of items and configuration, returns optimized packs with performance metrics
    /// </summary>
    /// <param name="request">Pack planning request containing items and configuration</param>
    /// <returns>Pack planning results with performance metrics</returns>
    /// <response code="200">Successfully planned packs</response>
    /// <response code="400">Invalid request data</response>
    /// <response code="500">Internal server error during processing</response>
    [HttpPost("pack")]
    [ProducesResponseType(typeof(PackPlanningResponse), StatusCodes.Status200OK)]
    [ProducesResponseType(typeof(ErrorResponse), StatusCodes.Status400BadRequest)]
    [ProducesResponseType(typeof(ErrorResponse), StatusCodes.Status500InternalServerError)]
    public async Task<ActionResult<PackPlanningResponse>> PlanPacks([FromBody] PackPlanningRequest request)
    {
        await Task.Delay(1);
        try
        {
            _logger.LogInformation("Received pack planning request with {ItemCount} items", request.Items?.Count ?? 0);

            // Validate request
            if (request.Items == null || request.Items.Count == 0)
            {
                _logger.LogWarning("Pack planning request received with no items");
                return BadRequest(new ErrorResponse
                {
                    Error = "Items list cannot be null or empty",
                    Details = "At least one item must be provided for pack planning"
                });
            }

            // Convert API models to domain models
            var items = request.Items.Select(item => new Item(
                item.Id,
                item.Length,
                item.Quantity,
                item.Weight
            )).ToList();

            var config = new PackPlannerConfig
            {
                Order = SortOrderExtensions.ParseSortOrder(request.Configuration?.SortOrder ?? "NATURAL"),
                MaxItemsPerPack = request.Configuration?.MaxItemsPerPack ?? 100,
                MaxWeightPerPack = request.Configuration?.MaxWeightPerPack ?? 200.0,
                Type = PackStrategyFactory.ParseStrategyType(request.Configuration?.StrategyType ?? "BLOCKING"),
                ThreadCount = request.Configuration?.ThreadCount ?? 4
            };

            // Execute pack planning
            var result = _packPlanner.PlanPacks(config, items);

            // Convert domain result to API response
            var response = new PackPlanningResponse
            {
                Success = true,
                Packs = result.Packs.Select(pack => new PackResponse
                {
                    PackNumber = pack.PackNumber,
                    Items = pack.Items.Select(item => new ItemResponse
                    {
                        Id = item.Id,
                        Length = item.Length,
                        Quantity = item.Quantity,
                        Weight = item.Weight,
                        TotalWeight = item.TotalWeight
                    }).ToList(),
                    TotalItems = pack.TotalItems,
                    TotalWeight = pack.TotalWeight,
                    PackLength = pack.PackLength,
                    IsEmpty = pack.IsEmpty
                }).ToList(),
                Metrics = new PerformanceMetrics
                {
                    SortingTimeMs = result.SortingTime,
                    PackingTimeMs = result.PackingTime,
                    TotalTimeMs = result.TotalTime,
                    TotalItems = result.TotalItems,
                    UtilizationPercent = result.UtilizationPercent,
                    StrategyUsed = result.StrategyName,
                    PackCount = result.Packs.Count(p => !p.IsEmpty)
                },
                Configuration = new ConfigurationResponse
                {
                    SortOrder = config.Order.ToShortString(),
                    MaxItemsPerPack = config.MaxItemsPerPack,
                    MaxWeightPerPack = config.MaxWeightPerPack,
                    StrategyType = PackStrategyFactory.StrategyTypeToString(config.Type),
                    ThreadCount = config.ThreadCount
                }
            };

            _logger.LogInformation("Pack planning completed successfully. Created {PackCount} packs in {TotalTime}ms",
                response.Metrics.PackCount, response.Metrics.TotalTimeMs);

            return Ok(response);
        }
        catch (ArgumentException ex)
        {
            _logger.LogWarning(ex, "Invalid argument in pack planning request");
            return BadRequest(new ErrorResponse
            {
                Error = "Invalid request parameters",
                Details = ex.Message
            });
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Unexpected error during pack planning");
            return StatusCode(500, new ErrorResponse
            {
                Error = "Internal server error",
                Details = "An unexpected error occurred while processing the request"
            });
        }
    }

    /// <summary>
    /// Health check endpoint
    /// Returns the current health status of the API
    /// </summary>
    /// <returns>Health status information</returns>
    /// <response code="200">Service is healthy</response>
    [HttpGet("health")]
    [ProducesResponseType(typeof(HealthResponse), StatusCodes.Status200OK)]
    public ActionResult<HealthResponse> GetHealth()
    {
        var response = new HealthResponse
        {
            Status = "Healthy",
            Timestamp = DateTime.UtcNow,
            Version = GetApiVersion(),
            Uptime = GetUptime()
        };

        return Ok(response);
    }

    /// <summary>
    /// Get available packing strategies
    /// Returns a list of all supported packing strategies
    /// </summary>
    /// <returns>List of available packing strategies</returns>
    /// <response code="200">Successfully retrieved strategies</response>
    [HttpGet("strategies")]
    [ProducesResponseType(typeof(StrategiesResponse), StatusCodes.Status200OK)]
    public ActionResult<StrategiesResponse> GetStrategies()
    {
        var strategies = Enum.GetValues<StrategyType>()
            .Select(strategy => new StrategyInfo
            {
                Name = PackStrategyFactory.StrategyTypeToString(strategy),
                DisplayName = strategy.ToString(),
                Description = GetStrategyDescription(strategy)
            })
            .ToList();

        var response = new StrategiesResponse
        {
            Strategies = strategies,
            DefaultStrategy = PackStrategyFactory.StrategyTypeToString(StrategyType.Blocking)
        };

        return Ok(response);
    }

    /// <summary>
    /// Get available sort orders
    /// Returns a list of all supported item sorting orders
    /// </summary>
    /// <returns>List of available sort orders</returns>
    /// <response code="200">Successfully retrieved sort orders</response>
    [HttpGet("sort-orders")]
    [ProducesResponseType(typeof(SortOrdersResponse), StatusCodes.Status200OK)]
    public ActionResult<SortOrdersResponse> GetSortOrders()
    {
        var sortOrders = Enum.GetValues<SortOrder>()
            .Select(order => new SortOrderInfo
            {
                Name = SortOrderExtensions.ParseSortOrder(order.ToString()).ToString().ToUpperInvariant(),
                ShortName = order.ToShortString(),
                DisplayName = order.ToString(),
                Description = GetSortOrderDescription(order)
            })
            .ToList();

        var response = new SortOrdersResponse
        {
            SortOrders = sortOrders,
            DefaultSortOrder = SortOrder.Natural.ToString().ToUpperInvariant()
        };

        return Ok(response);
    }

    /// <summary>
    /// Get API version information
    /// Returns current API version and build information
    /// </summary>
    /// <returns>API version information</returns>
    /// <response code="200">Successfully retrieved version information</response>
    [HttpGet("version")]
    [ProducesResponseType(typeof(VersionResponse), StatusCodes.Status200OK)]
    public ActionResult<VersionResponse> GetVersion()
    {
        var response = new VersionResponse
        {
            ApiVersion = GetApiVersion(),
            BuildDate = GetBuildDate(),
            Environment = Environment.GetEnvironmentVariable("ASPNETCORE_ENVIRONMENT") ?? "Production",
            DotNetVersion = Environment.Version.ToString()
        };

        return Ok(response);
    }

    [HttpPost("pack/async")]
    [ProducesResponseType(typeof(AsyncJobResponse), StatusCodes.Status202Accepted)]
    public async Task<ActionResult<AsyncJobResponse>> StartAsyncJob([FromBody] PackPlanningRequest request)
    {
        await Task.Delay(1);
        try
        {
            if (request.Items == null || request.Items.Count == 0)
                return BadRequest(new ErrorResponse { Error = "Items list cannot be empty" });

            var jobId = Guid.NewGuid().ToString();
            var job = new AsyncPackJob
            {
                JobId = jobId,
                Status = JobStatus.Pending,
                Request = request,
                CreatedAt = DateTime.UtcNow
            };

            _jobs.TryAdd(jobId, job);

            // Start background processing
            _ = Task.Run(async () => await ProcessJobAsync(jobId));

            return Accepted(new AsyncJobResponse
            {
                JobId = jobId,
                Status = JobStatus.Pending.ToString(),
                CreatedAt = job.CreatedAt,
                StatusUrl = Url.ActionLink(nameof(GetJobStatus), values: new { jobId }),
                ResultUrl = Url.ActionLink(nameof(GetJobResult), values: new { jobId })
            });
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to start async job");
            return StatusCode(500, new ErrorResponse { Error = "Job creation failed" });
        }
    }

    [HttpGet("pack/status/{jobId}")]
    [ProducesResponseType(typeof(JobStatusResponse), StatusCodes.Status200OK)]
    public ActionResult<JobStatusResponse> GetJobStatus(string jobId)
    {
        if (!_jobs.TryGetValue(jobId, out var job))
            return NotFound(new ErrorResponse { Error = "Job not found" });

        return Ok(new JobStatusResponse
        {
            JobId = jobId,
            Status = job.Status.ToString(),
            CreatedAt = job.CreatedAt,
            StartedAt = job.StartedAt,
            CompletedAt = job.CompletedAt,
            Progress = CalculateProgress(job.Status),
            EstimatedTimeRemaining = CalculateRemainingTime(job),
            ResultUrl = job.Status == JobStatus.Completed
                ? Url.ActionLink(nameof(GetJobResult), values: new { jobId })
                : null,
            ErrorMessage = job.ErrorMessage
        });
    }

    [HttpGet("pack/result/{jobId}")]
    [ProducesResponseType(typeof(PackPlanningResponse), StatusCodes.Status200OK)]
    public ActionResult<PackPlanningResponse> GetJobResult(string jobId)
    {
        if (!_jobs.TryGetValue(jobId, out var job))
            return NotFound(new ErrorResponse { Error = "Job not found" });

        if (job.Status != JobStatus.Completed)
            return Conflict(new ErrorResponse { Error = "Job not completed" });

        return Ok(job.Result ?? throw new Exception("Completed job missing results"));
    }

    [HttpDelete("pack/job/{jobId}")]
    [ProducesResponseType(typeof(JobDeletionResponse), StatusCodes.Status200OK)]
    public ActionResult<JobDeletionResponse> CancelJob(string jobId)
    {
        if (!_jobs.TryRemove(jobId, out var job))
            return NotFound(new ErrorResponse { Error = "Job not found" });

        return Ok(new JobDeletionResponse
        {
            JobId = jobId,
            PreviousStatus = job.Status.ToString(),
            DeletedAt = DateTime.UtcNow,
            Message = job.Status == JobStatus.Running
                ? "Job cancellation requested"
                : "Job removed"
        });
    }

    [HttpGet("rate-limits")]
    [ProducesResponseType(typeof(RateLimitInfoResponse), StatusCodes.Status200OK)]
    public ActionResult<RateLimitInfoResponse> GetRateLimitInfo()
    {
        // This would need to be injected if you want to access the actual configuration
        var response = new RateLimitInfoResponse
        {
            Enabled = true, // You'd get this from configuration
            Policies = new Dictionary<string, RateLimitPolicyInfo>
            {
                ["PackPlanning"] = new() { RequestLimit = 10, WindowSizeMinutes = 1, Description = "Main pack planning endpoint" },
                ["AsyncPackPlanning"] = new() { RequestLimit = 5, WindowSizeMinutes = 1, Description = "Async pack planning endpoint" },
                ["HealthCheck"] = new() { RequestLimit = 60, WindowSizeMinutes = 1, Description = "Health check endpoint" },
                ["Information"] = new() { RequestLimit = 30, WindowSizeMinutes = 1, Description = "Information endpoints" },
                ["JobManagement"] = new() { RequestLimit = 20, WindowSizeMinutes = 1, Description = "Job management endpoints" }
            },
            Headers = new List<string>
            {
                "X-RateLimit-Limit: Maximum requests allowed",
                "X-RateLimit-Remaining: Requests remaining in current window",
                "X-RateLimit-Reset: Unix timestamp when window resets",
                "X-RateLimit-Window: Window size (e.g., '1m')",
                "Retry-After: Seconds to wait before retrying (on 429 responses)"
            }
        };

        return Ok(response);
    }

    #region Private Helper Methods

    /// <summary>
    /// Gets the current API version from assembly information
    /// </summary>
    /// <returns>API version string</returns>
    private static string GetApiVersion()
    {
        var assembly = System.Reflection.Assembly.GetExecutingAssembly();
        var version = assembly.GetName().Version;
        return version?.ToString() ?? "1.0.0.0";
    }

    /// <summary>
    /// Gets the build date of the current assembly
    /// </summary>
    /// <returns>Build date string</returns>
    private static string GetBuildDate()
    {
        var assembly = System.Reflection.Assembly.GetExecutingAssembly();
        var fileInfo = new FileInfo(assembly.Location);
        return fileInfo.LastWriteTime.ToString("yyyy-MM-dd HH:mm:ss UTC");
    }

    /// <summary>
    /// Gets the application uptime
    /// </summary>
    /// <returns>Uptime string</returns>
    private static string GetUptime()
    {
        var uptime = DateTime.UtcNow - Process.GetCurrentProcess().StartTime.ToUniversalTime();
        return $"{uptime.Days}d {uptime.Hours}h {uptime.Minutes}m {uptime.Seconds}s";
    }

    /// <summary>
    /// Gets description for a packing strategy
    /// </summary>
    /// <param name="strategy">Strategy type</param>
    /// <returns>Strategy description</returns>
    private static string GetStrategyDescription(StrategyType strategy)
    {
        return strategy switch
        {
            StrategyType.Blocking => "Sequential processing strategy that processes items one by one",
            StrategyType.Parallel => "Parallel processing strategy that uses multiple threads for improved performance",
            _ => "Unknown strategy"
        };
    }

    /// <summary>
    /// Gets description for a sort order
    /// </summary>
    /// <param name="order">Sort order</param>
    /// <returns>Sort order description</returns>
    private static string GetSortOrderDescription(SortOrder order)
    {
        return order switch
        {
            SortOrder.Natural => "Keep items in their original order",
            SortOrder.ShortToLong => "Sort items by length from shortest to longest",
            SortOrder.LongToShort => "Sort items by length from longest to shortest",
            _ => "Unknown sort order"
        };
    }

    private async Task ProcessJobAsync(string jobId)
    {
        if (!_jobs.TryGetValue(jobId, out var job)) return;

        try
        {
            job.Status = JobStatus.Running;
            job.StartedAt = DateTime.UtcNow;

            // Reuse sync logic in async context
            var items = job.Request.Items.Select(i => new Item(i.Id, i.Length, i.Quantity, i.Weight)).ToList();
            var config = new PackPlannerConfig { /* ... */ };

            job.Result = await Task.Run(() =>
            {
                var domainResult = _packPlanner.PlanPacks(config, items);

                // Convert domain result to API response
                return new PackPlanningResponse
                {
                    Success = true,
                    Packs = domainResult.Packs.Select(pack => new PackResponse
                    {
                        PackNumber = pack.PackNumber,
                        Items = pack.Items.Select(item => new ItemResponse
                        {
                            Id = item.Id,
                            Length = item.Length,
                            Quantity = item.Quantity,
                            Weight = item.Weight,
                            TotalWeight = item.TotalWeight
                        }).ToList(),
                        TotalItems = pack.TotalItems,
                        TotalWeight = pack.TotalWeight,
                        PackLength = pack.PackLength,
                        IsEmpty = pack.IsEmpty
                    }).ToList(),
                    Metrics = new PerformanceMetrics
                    {
                        SortingTimeMs = domainResult.SortingTime,
                        PackingTimeMs = domainResult.PackingTime,
                        TotalTimeMs = domainResult.TotalTime,
                        TotalItems = domainResult.TotalItems,
                        UtilizationPercent = domainResult.UtilizationPercent,
                        StrategyUsed = domainResult.StrategyName,
                        PackCount = domainResult.Packs.Count(p => !p.IsEmpty)
                    },
                    Configuration = new ConfigurationResponse
                    {
                        SortOrder = config.Order.ToShortString(),
                        MaxItemsPerPack = config.MaxItemsPerPack,
                        MaxWeightPerPack = config.MaxWeightPerPack,
                        StrategyType = PackStrategyFactory.StrategyTypeToString(config.Type),
                        ThreadCount = config.ThreadCount
                    }
                };
            });

            job.Status = JobStatus.Completed;
            job.CompletedAt = DateTime.UtcNow;
        }
        catch (Exception ex)
        {
            job.Status = JobStatus.Failed;
            job.ErrorMessage = ex.Message;
            job.CompletedAt = DateTime.UtcNow;
            _logger.LogError(ex, "Job {JobId} failed", jobId);
        }
    }

    private static double CalculateProgress(JobStatus status) => status switch
    {
        JobStatus.Pending => 0,
        JobStatus.Running => 50,
        JobStatus.Completed => 100,
        JobStatus.Failed => 100,
        _ => 0
    };

    private static TimeSpan? CalculateRemainingTime(AsyncPackJob job)
    {
        if (job.Status != JobStatus.Running || !job.StartedAt.HasValue)
            return null;

        var elapsed = DateTime.UtcNow - job.StartedAt.Value;
        var estimatedTotal = TimeSpan.FromSeconds(job.Request.Items.Count * 0.1); // 100ms per item
        return estimatedTotal - elapsed;
    }

    #endregion
}
