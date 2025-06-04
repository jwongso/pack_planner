namespace PackPlanner;

/// <summary>
/// Enumeration for different packing strategy types
/// </summary>
public enum StrategyType
{
    /// <summary>
    /// Sequential blocking strategy
    /// </summary>
    Blocking,

    /// <summary>
    /// Parallel processing strategy
    /// </summary>
    Parallel
}

/// <summary>
/// Strategy interface for different packing algorithms
/// </summary>
public interface IPackStrategy
{
    /// <summary>
    /// Pack items using the specific strategy
    /// </summary>
    /// <param name="items">Items to pack</param>
    /// <param name="maxItems">Maximum items per pack</param>
    /// <param name="maxWeight">Maximum weight per pack</param>
    /// <returns>List of packed items</returns>
    List<Pack> PackItems(IReadOnlyList<Item> items, int maxItems, double maxWeight);

    /// <summary>
    /// Get strategy name for identification
    /// </summary>
    string Name { get; }
}

/// <summary>
/// Factory for creating pack strategies
/// </summary>
public static class PackStrategyFactory
{
    /// <summary>
    /// Create a pack strategy
    /// </summary>
    /// <param name="type">Strategy type to create</param>
    /// <param name="threadCount">Number of threads for parallel strategy (ignored for others)</param>
    /// <returns>Created strategy</returns>
    public static IPackStrategy CreateStrategy(StrategyType type, int threadCount = 4)
    {
        return type switch
        {
            StrategyType.Blocking => new BlockingPackStrategy(),
            StrategyType.Parallel => new ParallelPackStrategy(threadCount),
            _ => new BlockingPackStrategy()
        };
    }

    /// <summary>
    /// Parse strategy type from string
    /// </summary>
    /// <param name="str">String representation</param>
    /// <returns>Parsed strategy type</returns>
    public static StrategyType ParseStrategyType(string str)
    {
        return str.ToUpperInvariant() switch
        {
            "PARALLEL" => StrategyType.Parallel,
            "BLOCKING" => StrategyType.Blocking,
            _ => StrategyType.Blocking // default
        };
    }

    /// <summary>
    /// Convert strategy type to string
    /// </summary>
    /// <param name="type">Strategy type</param>
    /// <returns>String representation</returns>
    public static string StrategyTypeToString(StrategyType type)
    {
        return type switch
        {
            StrategyType.Blocking => "BLOCKING",
            StrategyType.Parallel => "PARALLEL",
            _ => "BLOCKING"
        };
    }
}
