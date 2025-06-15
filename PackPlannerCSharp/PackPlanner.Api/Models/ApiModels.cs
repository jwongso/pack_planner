using System.ComponentModel.DataAnnotations;

namespace PackPlanner.Api.Models;

#region Request Models

/// <summary>
/// Main request model for pack planning operations
/// Contains items to pack and configuration options
/// </summary>
public class PackPlanningRequest
{
    /// <summary>
    /// List of items to be packed
    /// </summary>
    [Required]
    public List<ItemRequest> Items { get; set; } = new();

    /// <summary>
    /// Configuration options for the pack planning process
    /// </summary>
    public PackPlanningConfiguration? Configuration { get; set; }
}

/// <summary>
/// Represents an item in the pack planning request
/// </summary>
public class ItemRequest
{
    /// <summary>
    /// Unique identifier for the item
    /// </summary>
    [Required]
    public int Id { get; set; }

    /// <summary>
    /// Length of the item
    /// </summary>
    [Required]
    [Range(1, int.MaxValue, ErrorMessage = "Length must be positive")]
    public int Length { get; set; }

    /// <summary>
    /// Quantity of this item
    /// </summary>
    [Required]
    [Range(1, int.MaxValue, ErrorMessage = "Quantity must be positive")]
    public int Quantity { get; set; }

    /// <summary>
    /// Weight per piece of the item
    /// </summary>
    [Required]
    [Range(0.0, double.MaxValue, ErrorMessage = "Weight must be non-negative")]
    public double Weight { get; set; }
}

/// <summary>
/// Configuration options for pack planning
/// </summary>
public class PackPlanningConfiguration
{
    /// <summary>
    /// Sort order for items (NATURAL, SHORT_TO_LONG, LONG_TO_SHORT)
    /// </summary>
    public string SortOrder { get; set; } = "NATURAL";

    /// <summary>
    /// Maximum number of items per pack
    /// </summary>
    [Range(1, int.MaxValue, ErrorMessage = "MaxItemsPerPack must be positive")]
    public int MaxItemsPerPack { get; set; } = 100;

    /// <summary>
    /// Maximum weight per pack
    /// </summary>
    [Range(0.1, double.MaxValue, ErrorMessage = "MaxWeightPerPack must be positive")]
    public double MaxWeightPerPack { get; set; } = 200.0;

    /// <summary>
    /// Strategy type to use (BLOCKING, PARALLEL)
    /// </summary>
    public string StrategyType { get; set; } = "BLOCKING";

    /// <summary>
    /// Number of threads for parallel strategy
    /// </summary>
    [Range(1, 32, ErrorMessage = "ThreadCount must be between 1 and 32")]
    public int ThreadCount { get; set; } = 4;
}

#endregion

#region Response Models

/// <summary>
/// Main response model for pack planning operations
/// Contains the packed results and performance metrics
/// </summary>
public class PackPlanningResponse
{
    /// <summary>
    /// Indicates if the operation was successful
    /// </summary>
    public bool Success { get; set; }

    /// <summary>
    /// List of created packs
    /// </summary>
    public List<PackResponse> Packs { get; set; } = new();

    /// <summary>
    /// Performance metrics for the operation
    /// </summary>
    public PerformanceMetrics Metrics { get; set; } = new();

    /// <summary>
    /// Configuration used for the operation
    /// </summary>
    public ConfigurationResponse Configuration { get; set; } = new();
}

/// <summary>
/// Represents a pack in the response
/// </summary>
public class PackResponse
{
    /// <summary>
    /// Pack identifier number
    /// </summary>
    public int PackNumber { get; set; }

    /// <summary>
    /// Items contained in this pack
    /// </summary>
    public List<ItemResponse> Items { get; set; } = new();

    /// <summary>
    /// Total number of items in the pack
    /// </summary>
    public int TotalItems { get; set; }

    /// <summary>
    /// Total weight of the pack
    /// </summary>
    public double TotalWeight { get; set; }

    /// <summary>
    /// Maximum length of any item in the pack
    /// </summary>
    public int PackLength { get; set; }

    /// <summary>
    /// Indicates if the pack is empty
    /// </summary>
    public bool IsEmpty { get; set; }
}

/// <summary>
/// Represents an item in the response
/// </summary>
public class ItemResponse
{
    /// <summary>
    /// Item identifier
    /// </summary>
    public int Id { get; set; }

    /// <summary>
    /// Item length
    /// </summary>
    public int Length { get; set; }

    /// <summary>
    /// Item quantity
    /// </summary>
    public int Quantity { get; set; }

    /// <summary>
    /// Weight per piece
    /// </summary>
    public double Weight { get; set; }

    /// <summary>
    /// Total weight for this item (quantity * weight)
    /// </summary>
    public double TotalWeight { get; set; }
}

/// <summary>
/// Performance metrics for pack planning operations
/// </summary>
public class PerformanceMetrics
{
    /// <summary>
    /// Time spent sorting items in milliseconds
    /// </summary>
    public double SortingTimeMs { get; set; }

    /// <summary>
    /// Time spent packing items in milliseconds
    /// </summary>
    public double PackingTimeMs { get; set; }

    /// <summary>
    /// Total operation time in milliseconds
    /// </summary>
    public double TotalTimeMs { get; set; }

    /// <summary>
    /// Total number of items processed
    /// </summary>
    public int TotalItems { get; set; }

    /// <summary>
    /// Pack utilization percentage
    /// </summary>
    public double UtilizationPercent { get; set; }

    /// <summary>
    /// Name of the strategy used
    /// </summary>
    public string StrategyUsed { get; set; } = string.Empty;

    /// <summary>
    /// Number of packs created
    /// </summary>
    public int PackCount { get; set; }
}

/// <summary>
/// Configuration information in the response
/// </summary>
public class ConfigurationResponse
{
    /// <summary>
    /// Sort order used
    /// </summary>
    public string SortOrder { get; set; } = string.Empty;

    /// <summary>
    /// Maximum items per pack
    /// </summary>
    public int MaxItemsPerPack { get; set; }

    /// <summary>
    /// Maximum weight per pack
    /// </summary>
    public double MaxWeightPerPack { get; set; }

    /// <summary>
    /// Strategy type used
    /// </summary>
    public string StrategyType { get; set; } = string.Empty;

    /// <summary>
    /// Number of threads used
    /// </summary>
    public int ThreadCount { get; set; }
}

/// <summary>
/// Health check response model
/// </summary>
public class HealthResponse
{
    /// <summary>
    /// Current health status
    /// </summary>
    public string Status { get; set; } = string.Empty;

    /// <summary>
    /// Timestamp of the health check
    /// </summary>
    public DateTime Timestamp { get; set; }

    /// <summary>
    /// API version
    /// </summary>
    public string Version { get; set; } = string.Empty;

    /// <summary>
    /// Application uptime
    /// </summary>
    public string Uptime { get; set; } = string.Empty;
}

/// <summary>
/// Available strategies response model
/// </summary>
public class StrategiesResponse
{
    /// <summary>
    /// List of available strategies
    /// </summary>
    public List<StrategyInfo> Strategies { get; set; } = new();

    /// <summary>
    /// Default strategy name
    /// </summary>
    public string DefaultStrategy { get; set; } = string.Empty;
}

/// <summary>
/// Information about a packing strategy
/// </summary>
public class StrategyInfo
{
    /// <summary>
    /// Strategy name (API identifier)
    /// </summary>
    public string Name { get; set; } = string.Empty;

    /// <summary>
    /// Human-readable display name
    /// </summary>
    public string DisplayName { get; set; } = string.Empty;

    /// <summary>
    /// Strategy description
    /// </summary>
    public string Description { get; set; } = string.Empty;
}

/// <summary>
/// Available sort orders response model
/// </summary>
public class SortOrdersResponse
{
    /// <summary>
    /// List of available sort orders
    /// </summary>
    public List<SortOrderInfo> SortOrders { get; set; } = new();

    /// <summary>
    /// Default sort order name
    /// </summary>
    public string DefaultSortOrder { get; set; } = string.Empty;
}

/// <summary>
/// Information about a sort order
/// </summary>
public class SortOrderInfo
{
    /// <summary>
    /// Sort order name (API identifier)
    /// </summary>
    public string Name { get; set; } = string.Empty;

    /// <summary>
    /// Short name abbreviation
    /// </summary>
    public string ShortName { get; set; } = string.Empty;

    /// <summary>
    /// Human-readable display name
    /// </summary>
    public string DisplayName { get; set; } = string.Empty;

    /// <summary>
    /// Sort order description
    /// </summary>
    public string Description { get; set; } = string.Empty;
}

/// <summary>
/// API version information response model
/// </summary>
public class VersionResponse
{
    /// <summary>
    /// Current API version
    /// </summary>
    public string ApiVersion { get; set; } = string.Empty;

    /// <summary>
    /// Build date
    /// </summary>
    public string BuildDate { get; set; } = string.Empty;

    /// <summary>
    /// Current environment
    /// </summary>
    public string Environment { get; set; } = string.Empty;

    /// <summary>
    /// .NET version
    /// </summary>
    public string DotNetVersion { get; set; } = string.Empty;
}

/// <summary>
/// Error response model for API errors
/// </summary>
public class ErrorResponse
{
    /// <summary>
    /// Error message
    /// </summary>
    public string Error { get; set; } = string.Empty;

    /// <summary>
    /// Additional error details
    /// </summary>
    public string Details { get; set; } = string.Empty;

    /// <summary>
    /// Timestamp of the error
    /// </summary>
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
}

#endregion

#region Async Job Models

/// <summary>
/// Job status enumeration for async operations
/// </summary>
public enum JobStatus
{
    /// <summary>
    /// Job is queued and waiting to start
    /// </summary>
    Pending,

    /// <summary>
    /// Job is currently being processed
    /// </summary>
    Running,

    /// <summary>
    /// Job completed successfully
    /// </summary>
    Completed,

    /// <summary>
    /// Job failed with an error
    /// </summary>
    Failed,

    /// <summary>
    /// Job was cancelled by user request
    /// </summary>
    Cancelled
}

/// <summary>
/// Internal job tracking model for async operations
/// </summary>
internal class AsyncPackJob
{
    /// <summary>
    /// Unique job identifier
    /// </summary>
    public required string JobId { get; set; }

    /// <summary>
    /// Current job status
    /// </summary>
    public JobStatus Status { get; set; }

    /// <summary>
    /// Original pack planning request
    /// </summary>
    public required PackPlanningRequest Request { get; set; }

    /// <summary>
    /// Job creation timestamp
    /// </summary>
    public DateTime CreatedAt { get; set; }

    /// <summary>
    /// Job start timestamp (when processing began)
    /// </summary>
    public DateTime? StartedAt { get; set; }

    /// <summary>
    /// Job completion timestamp
    /// </summary>
    public DateTime? CompletedAt { get; set; }

    /// <summary>
    /// Cached result for completed jobs
    /// </summary>
    public PackPlanningResponse? Result { get; set; }

    /// <summary>
    /// Error message for failed jobs
    /// </summary>
    public string? ErrorMessage { get; set; }
}

/// <summary>
/// Response model for async job creation
/// </summary>
public class AsyncJobResponse
{
    /// <summary>
    /// Unique job identifier for tracking
    /// </summary>
    public string JobId { get; set; } = string.Empty;

    /// <summary>
    /// Initial job status (typically "Pending")
    /// </summary>
    public string Status { get; set; } = string.Empty;

    /// <summary>
    /// Job creation timestamp
    /// </summary>
    public DateTime CreatedAt { get; set; }

    /// <summary>
    /// Estimated completion time
    /// </summary>
    public DateTime EstimatedCompletionTime { get; set; }

    /// <summary>
    /// URL to poll for job status
    /// </summary>
    public string StatusUrl { get; set; } = string.Empty;

    /// <summary>
    /// URL to retrieve results when completed
    /// </summary>
    public string ResultUrl { get; set; } = string.Empty;
}

/// <summary>
/// Response model for job status polling
/// </summary>
public class JobStatusResponse
{
    /// <summary>
    /// Job identifier
    /// </summary>
    public string JobId { get; set; } = string.Empty;

    /// <summary>
    /// Current job status
    /// </summary>
    public string Status { get; set; } = string.Empty;

    /// <summary>
    /// Job creation timestamp
    /// </summary>
    public DateTime CreatedAt { get; set; }

    /// <summary>
    /// Job start timestamp (null if not started)
    /// </summary>
    public DateTime? StartedAt { get; set; }

    /// <summary>
    /// Job completion timestamp (null if not completed)
    /// </summary>
    public DateTime? CompletedAt { get; set; }

    /// <summary>
    /// Job progress percentage (0-100)
    /// </summary>
    public double Progress { get; set; }

    /// <summary>
    /// Estimated time remaining for completion
    /// </summary>
    public TimeSpan? EstimatedTimeRemaining { get; set; }

    /// <summary>
    /// URL to retrieve results (only present when completed)
    /// </summary>
    public string? ResultUrl { get; set; }

    /// <summary>
    /// Error message for failed jobs
    /// </summary>
    public string? ErrorMessage { get; set; }
}

/// <summary>
/// Response model for job deletion/cancellation
/// </summary>
public class JobDeletionResponse
{
    /// <summary>
    /// Job identifier that was deleted
    /// </summary>
    public string JobId { get; set; } = string.Empty;

    /// <summary>
    /// Status of the job before deletion
    /// </summary>
    public string PreviousStatus { get; set; } = string.Empty;

    /// <summary>
    /// Timestamp when job was deleted
    /// </summary>
    public DateTime DeletedAt { get; set; }

    /// <summary>
    /// Descriptive message about the deletion
    /// </summary>
    public string Message { get; set; } = string.Empty;
}

/// <summary>
/// Rate limiting information response model
/// </summary>
public class RateLimitInfoResponse
{
    /// <summary>
    /// Whether rate limiting is enabled
    /// </summary>
    public bool Enabled { get; set; }

    /// <summary>
    /// Rate limiting policies by endpoint category
    /// </summary>
    public Dictionary<string, RateLimitPolicyInfo> Policies { get; set; } = new();

    /// <summary>
    /// Description of rate limiting headers
    /// </summary>
    public List<string> Headers { get; set; } = new();
}

/// <summary>
/// Rate limiting policy information
/// </summary>
public class RateLimitPolicyInfo
{
    /// <summary>
    /// Maximum requests allowed per window
    /// </summary>
    public int RequestLimit { get; set; }

    /// <summary>
    /// Window size in minutes
    /// </summary>
    public int WindowSizeMinutes { get; set; }

    /// <summary>
    /// Policy description
    /// </summary>
    public string Description { get; set; } = string.Empty;
}

#endregion
