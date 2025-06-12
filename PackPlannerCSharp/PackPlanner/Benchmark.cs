namespace PackPlanner;

/// <summary>
/// Results of a single benchmark test
/// </summary>
public record BenchmarkResult
{
    public required int Size { get; init; }
    public required string Order { get; init; }
    public required string Strategy { get; init; }
    public required int NumThreads { get; init; }
    public required double SortingTime { get; init; }
    public required double PackingTime { get; init; }
    public required double TotalTime { get; init; }
    public required long ItemsPerSecond { get; init; }
    public required int TotalPacks { get; init; }
    public required double UtilizationPercent { get; init; }
}

/// <summary>
/// Benchmark class for performance testing
/// </summary>
public class Benchmark
{
    private readonly PackPlanner _planner;
    private readonly Timer _totalTimer;

    // Default benchmark configuration
    private const int MaxItemsPerPack = 100;
    private const double MaxWeightPerPack = 200.0;

    // Benchmark sizes
    private static readonly int[] BenchmarkSizes = { 100000, 1000000, 5000000, 10000000, 20000000 };
    private static readonly SortOrder[] SortOrders = { SortOrder.Natural, SortOrder.LongToShort, SortOrder.ShortToLong };
    private static readonly StrategyType[] PackingStrategies = { StrategyType.Blocking, StrategyType.Parallel };
    private static readonly int[] ThreadCounts = { 0 }; // 0 means use processor count

    /// <summary>
    /// Initializes a new instance of the Benchmark class
    /// </summary>
    public Benchmark()
    {
        _planner = new PackPlanner();
        _totalTimer = new Timer();
    }

    /// <summary>
    /// Run comprehensive benchmark with different sizes and sort orders
    /// </summary>
    public void RunBenchmark()
    {
        Console.WriteLine("=== PERFORMANCE BENCHMARK ===");
        Console.WriteLine("Running C# Performance Benchmarks...");

        var allResults = new List<BenchmarkResult>();

        _totalTimer.Start();

        foreach (var strategy in PackingStrategies)
        {
            foreach (int threads in ThreadCounts)
            {
                // Skip thread variations for blocking strategy
                if (strategy == StrategyType.Blocking && threads > 0)
                {
                    continue;
                }

                foreach (var order in SortOrders)
                {
                    Console.Write($"Strategy: {PackStrategyFactory.StrategyTypeToString(strategy)}");
                    if (strategy == StrategyType.Parallel)
                    {
                        Console.Write($" (Threads: {(threads == 0 ? "Auto" : threads.ToString())})");
                    }
                    Console.WriteLine($", Order: {order.ToShortString()}");

                    Console.WriteLine("Size      Sort(ms)    Pack(ms)    Total(ms)   Items/sec   Packs       Util%");
                    Console.WriteLine("----------------------------------------------------------------------------");

                    foreach (int size in BenchmarkSizes)
                    {
                        var result = RunSingleBenchmark(size, order, strategy, threads);
                        allResults.Add(result);

                        Console.WriteLine($"{size,-10}{result.SortingTime,-12:F3}{result.PackingTime,-12:F3}" +
                                        $"{result.TotalTime,-12:F3}{result.ItemsPerSecond,-12}{result.TotalPacks,-12}" +
                                        $"{result.UtilizationPercent:F1}%");
                    }
                    Console.WriteLine();
                }
            }
        }

        double totalBenchmarkTime = _totalTimer.Stop();

        Console.WriteLine($"Total benchmark execution: {totalBenchmarkTime:F3} ms " +
                         $"({(long)(totalBenchmarkTime * 1000)} μs)");
    }

    /// <summary>
    /// Generate test data for benchmarking
    /// </summary>
    /// <param name="size">Number of items to generate</param>
    /// <returns>List of test items</returns>
    public static List<Item> GenerateTestData(int size)
    {
        var items = new List<Item>(size);
        
        var random = new Random(48); // Fixed seed for reproducible results
        
        for (int i = 0; i < size; i++)
        {
            int id = 1000 + i;
            int length = random.Next(500, 10001); // 500-10000
            int quantity = random.Next(10, 101);  // 10-100
            double weight;
            
            // 70% lightweight items (sorting matters)
            // 30% heavyweight items (always split)
            if (i % 10 < 7)
            {
                weight = random.NextDouble() * 5.5 + 0.5; // 0.5-6.0kg
            }
            else
            {
                weight = random.NextDouble() * 23.9 + 6.1; // 6.1-30.0kg
            }
            
            items.Add(new Item(id, length, quantity, weight));
        }
        
        return items;
    }

    /// <summary>
    /// Run single benchmark test
    /// </summary>
    /// <param name="size">Number of items to test</param>
    /// <param name="sortOrder">Sort order to use</param>
    /// <param name="strategy">Strategy type to use</param>
    /// <param name="numThreads">Number of threads for parallel strategy</param>
    /// <returns>Benchmark result</returns>
    public BenchmarkResult RunSingleBenchmark(int size, SortOrder sortOrder,
                                             StrategyType strategy, int numThreads)
    {
        // Generate test data
        var items = GenerateTestData(size);

        // Configure pack planner
        var config = new PackPlannerConfig
        {
            Order = sortOrder,
            MaxItemsPerPack = MaxItemsPerPack,
            MaxWeightPerPack = MaxWeightPerPack,
            Type = strategy,
            ThreadCount = numThreads
        };

        // Run pack planning
        var planResult = _planner.PlanPacks(config, items);

        // Calculate items per second
        long itemsPerSecond = 0;
        if (planResult.TotalTime > 0)
        {
            itemsPerSecond = (long)((planResult.TotalItems * 1000.0) / planResult.TotalTime);
        }

        return new BenchmarkResult
        {
            Size = size,
            Order = sortOrder.ToShortString(),
            Strategy = PackStrategyFactory.StrategyTypeToString(strategy),
            NumThreads = numThreads,
            SortingTime = planResult.SortingTime,
            PackingTime = planResult.PackingTime,
            TotalTime = planResult.TotalTime,
            ItemsPerSecond = itemsPerSecond,
            TotalPacks = planResult.Packs.Count,
            UtilizationPercent = planResult.UtilizationPercent
        };
    }

    /// <summary>
    /// Output benchmark results in tabular format
    /// </summary>
    /// <param name="results">Results to output</param>
    public static void OutputBenchmarkResults(IEnumerable<BenchmarkResult> results)
    {
        Console.WriteLine("Size    Strategy  Threads  Order    Sort(ms)  Pack(ms)  Total(ms) Items/sec Packs   Util%");
        Console.WriteLine("----------------------------------------------------------------------------------------");

        foreach (var result in results)
        {
            string threadsStr = result.Strategy == "Parallel" 
                ? (result.NumThreads == 0 ? "Auto" : result.NumThreads.ToString()) 
                : "-";

            Console.WriteLine($"{result.Size,-8}{result.Strategy,-10}{threadsStr,-9}{result.Order,-9}" +
                            $"{result.SortingTime,-10:F3}{result.PackingTime,-10:F3}{result.TotalTime,-10:F3}" +
                            $"{result.ItemsPerSecond,-10}{result.TotalPacks,-8}{result.UtilizationPercent:F1}%");
        }
    }
}
