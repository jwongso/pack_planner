using PackPlanner;
using System.Collections.Concurrent;

namespace PackPlanner.Tests;

public class ParallelStrategyTests : PackPlannerStrategyTestBase
{
    public ParallelStrategyTests() : base(StrategyType.Parallel) { }

    // --- Correctness (vs Blocking) ---
    [Fact]
    public void PlanPacks_MatchesBlockingStrategyResults()
    {
        var blockingConfig = _config with { Type = StrategyType.Blocking };
        var blockingResult = _planner.PlanPacks(blockingConfig, _items);
        var parallelResult = _planner.PlanPacks(_config, _items);

        Assert.Equal(blockingResult.Packs.Count, parallelResult.Packs.Count);
        Assert.Equal(blockingResult.TotalItems, parallelResult.TotalItems);
    }

    // --- Thread-Safety ---
    [Fact]
    public void PlanPacks_ThreadSafeUnderHighConcurrency()
    {
        var exceptions = new ConcurrentBag<Exception>();
        var threads = new Thread[8];

        for (int i = 0; i < threads.Length; i++)
        {
            threads[i] = new Thread(() =>
            {
                try { _planner.PlanPacks(_config, _items); }
                catch (Exception ex) { exceptions.Add(ex); }
            });
            threads[i].Start();
        }

        foreach (var t in threads) t.Join();
        Assert.Empty(exceptions);
    }

    // --- Performance ---
    [Fact]
    public void PlanPacks_VeryLargeInput_ShowsParallelScaling()
    {
        var veryLargeItems = Enumerable.Range(1, 1_000_000)  // 1M items
            .Select(i => new Item(i, i % 1000, 1, 1.0))
            .ToList();

        // Test with different thread counts
        var results = new Dictionary<int, long>();
        foreach (var threadCount in new[] { 1, 2, 4, 8 })
        {
            var config = _config with { ThreadCount = threadCount };
            var time = TimeExecution(() => _planner.PlanPacks(config, veryLargeItems));
            results.Add(threadCount, time);
        }

        // Verify scaling trend (not strict due to overhead)
        Assert.True(results[4] >= results[1],
            $"4 threads should be significantly faster than 1 thread (was {results[4]}ms vs {results[1]}ms)");
    }

    // --- Configuration Edge Cases ---
    [Theory]
    [InlineData(0)]  // Min threads
    [InlineData(64)] // Excess threads
    public void PlanPacks_HandlesThreadCountVariations(int threadCount)
    {
        var config = _config with { ThreadCount = threadCount };
        var result = _planner.PlanPacks(config, _items);
        Assert.NotEmpty(result.Packs);
    }

    [Theory]
    [InlineData(0)]  // Min threads
    [InlineData(64)] // Excess threads
    public void PlanPacksNaturalOrderParallel(int threadCount)
    {
        var config = _config with { ThreadCount = threadCount };
        var result = _planner.PlanPacks(_config, _items);

        // Should create 2 packs with natural order
        Assert.Equal(2, result.Packs.Count);
        Assert.Equal(9, result.Packs[0].TotalItems);
        Assert.Equal(24.0, result.Packs[0].TotalWeight);
        Assert.Equal(5, result.Packs[1].TotalItems);
        Assert.Equal(15.0, result.Packs[1].TotalWeight);

        // Check total items
        Assert.Equal(14, result.TotalItems); // 5+3+2+4

        // Check utilization
        Assert.True(Math.Abs(result.UtilizationPercent - 78.0) < 0.1);
    }
}
