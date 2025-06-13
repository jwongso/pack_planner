namespace PackPlanner;

/// <summary>
/// Configuration for the pack planning process
/// </summary>
public record PackPlannerConfig
{
    /// <summary>
    /// Sort order for items
    /// </summary>
    public SortOrder Order { get; init; } = SortOrder.Natural;

    /// <summary>
    /// Maximum items per pack
    /// </summary>
    public int MaxItemsPerPack { get; init; } = 100;

    /// <summary>
    /// Maximum weight per pack
    /// </summary>
    public double MaxWeightPerPack { get; init; } = 200.0;

    /// <summary>
    /// Strategy type to use
    /// </summary>
    public StrategyType Type { get; init; } = StrategyType.Blocking;

    /// <summary>
    /// Number of threads for parallel strategy
    /// </summary>
    public int ThreadCount { get; init; } = 4;
}

/// <summary>
/// Results of the pack planning process
/// </summary>
public record PackPlannerResult
{
    /// <summary>
    /// The resulting packs
    /// </summary>
    public required List<Pack> Packs { get; init; }

    /// <summary>
    /// Time spent sorting items (milliseconds)
    /// </summary>
    public required double SortingTime { get; init; }

    /// <summary>
    /// Time spent packing items (milliseconds)
    /// </summary>
    public required double PackingTime { get; init; }

    /// <summary>
    /// Total time for the operation (milliseconds)
    /// </summary>
    public required double TotalTime { get; init; }

    /// <summary>
    /// Total number of items processed
    /// </summary>
    public required int TotalItems { get; init; }

    /// <summary>
    /// Pack utilization percentage
    /// </summary>
    public required double UtilizationPercent { get; init; }

    /// <summary>
    /// Name of the strategy used
    /// </summary>
    public required string StrategyName { get; init; }
}

/// <summary>
/// Main class for planning how to pack items into packs
/// </summary>
public class PackPlanner
{
    private readonly Timer _timer;

    /// <summary>
    /// Initializes a new instance of the PackPlanner class
    /// </summary>
    public PackPlanner()
    {
        _timer = new Timer();
    }

    /// <summary>
    /// Plan packs with given configuration and items
    /// </summary>
    /// <param name="config">Configuration for planning</param>
    /// <param name="items">Items to pack</param>
    /// <returns>Results of the planning process</returns>
    public PackPlannerResult PlanPacks(PackPlannerConfig config, List<Item> items)
    {
        // Start total timing
        _timer.Start();

        // SAFETY: Validate and sanitize configuration
        var safeConfig = config with
        {
            MaxItemsPerPack = Math.Max(1, config.MaxItemsPerPack),
            MaxWeightPerPack = Math.Max(0.1, config.MaxWeightPerPack),
            ThreadCount = Math.Clamp(config.ThreadCount, 1, 32)
        };

        // Sort items
        var sortTimer = new Timer();

        SortOrder actualOrder = safeConfig.Order switch
        {
            SortOrder.ShortToLong => SortOrder.ShortToLongParallel,
            SortOrder.LongToShort => SortOrder.LongToShortParallel,
            _ => safeConfig.Order
        };


        sortTimer.Start();
        SortOrderExtensions.SortItems(items, safeConfig.Order);
        double sortingTime = sortTimer.Stop();

        // Create strategy
        var strategy = PackStrategyFactory.CreateStrategy(safeConfig.Type, safeConfig.ThreadCount);
        string strategyName = strategy.Name;

        // Pack items using selected strategy
        var packTimer = new Timer();
        packTimer.Start();
        var packs = strategy.PackItems(items, safeConfig.MaxItemsPerPack, safeConfig.MaxWeightPerPack);
        double packingTime = packTimer.Stop();

        double totalTime = _timer.Stop();

        // SAFETY: Calculate total items safely
        int totalItems = 0;
        foreach (var item in items)
        {
            // SAFETY: Skip negative quantities and avoid overflow
            if (item.Quantity > 0 && totalItems <= int.MaxValue - item.Quantity)
            {
                totalItems += item.Quantity;
            }
        }

        // Calculate utilization
        double utilizationPercent = CalculateUtilization(packs, safeConfig.MaxWeightPerPack);

        return new PackPlannerResult
        {
            Packs = packs,
            SortingTime = sortingTime,
            PackingTime = packingTime,
            TotalTime = totalTime,
            TotalItems = totalItems,
            UtilizationPercent = utilizationPercent,
            StrategyName = strategyName
        };
    }

    /// <summary>
    /// Output results to a stream
    /// </summary>
    /// <param name="packs">Packs to output</param>
    /// <param name="output">Output stream (defaults to Console.Out)</param>
    public static void OutputResults(IEnumerable<Pack> packs, TextWriter? output = null)
    {
        output ??= Console.Out;

        foreach (var pack in packs)
        {
            if (!pack.IsEmpty)
            {
                output.WriteLine(pack.ToString());
            }
        }
    }

    /// <summary>
    /// Calculate utilization percentage
    /// </summary>
    /// <param name="packs">Packs to calculate utilization for</param>
    /// <param name="maxWeight">Maximum weight per pack</param>
    /// <returns>Utilization percentage</returns>
    public static double CalculateUtilization(IEnumerable<Pack> packs, double maxWeight)
    {
        var nonEmptyPacks = packs.Where(p => !p.IsEmpty).ToList();
        
        if (nonEmptyPacks.Count == 0 || maxWeight <= 0.0) return 0.0;

        double totalWeight = 0.0;
        int validPacks = 0;

        foreach (var pack in nonEmptyPacks)
        {
            // SAFETY: Avoid potential floating-point overflow
            if (pack.TotalWeight >= 0.0 && totalWeight <= double.MaxValue - pack.TotalWeight)
            {
                totalWeight += pack.TotalWeight;
                validPacks++;
            }
        }

        if (validPacks == 0) return 0.0;

        double maxPossibleWeight = validPacks * maxWeight;

        // SAFETY: Avoid division by zero
        if (maxPossibleWeight <= 0.0) return 0.0;

        // SAFETY: Clamp result to valid percentage range
        return Math.Clamp((totalWeight / maxPossibleWeight) * 100.0, 0.0, 100.0);
    }
}
