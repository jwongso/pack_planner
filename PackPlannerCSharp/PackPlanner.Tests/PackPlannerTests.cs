using Xunit;
using PackPlanner;
using System.Diagnostics;

namespace PackPlanner.Tests;

/// <summary>
/// Pack Planner Tests
/// </summary>
public class PackPlannerTests
{
    private readonly global::PackPlanner.PackPlanner _planner;
    private readonly List<Item> _items;
    private readonly PackPlannerConfig _config;

    public PackPlannerTests()
    {
        // Common setup for pack planner tests
        _planner = new global::PackPlanner.PackPlanner();

        // Create some test items
        _items = new List<Item>
        {
            new Item(1, 100, 5, 2.0),  // total weight: 10.0
            new Item(2, 200, 3, 3.0),  // total weight: 9.0
            new Item(3, 300, 2, 5.0),  // total weight: 10.0
            new Item(4, 150, 4, 2.5)   // total weight: 10.0
        };

        // Default configuration
        _config = new PackPlannerConfig
        {
            Order = SortOrder.Natural,
            MaxItemsPerPack = 10,
            MaxWeightPerPack = 25.0
        };
    }

    [Fact]
    public void PlanPacksNaturalOrder()
    {
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
        Assert.True(Math.Abs(result.UtilizationPercent - 78.0) < 0.1); // (24.0+15.0)/(25.0*2) * 100
    }

    [Fact]
    public void PlanPacksShortToLong()
    {
        var config = _config with { Order = SortOrder.ShortToLong };
        var result = _planner.PlanPacks(config, _items);

        // Check that items were sorted by length
        var packs = result.Packs;
        Assert.Equal(2, packs.Count);

        // First pack should have shorter items
        var firstPackItems = packs[0].Items;
        Assert.Equal(100, firstPackItems[0].Length);

        // Verify total items and weights
        Assert.Equal(14, result.TotalItems);
    }

    [Fact]
    public void PlanPacksLongToShort()
    {
        var config = _config with { Order = SortOrder.LongToShort };
        var result = _planner.PlanPacks(config, _items);

        // Check that items were sorted by length
        var packs = result.Packs;
        Assert.Equal(2, packs.Count);

        // First pack should have longer items
        var firstPackItems = packs[0].Items;
        Assert.Equal(300, firstPackItems[0].Length);

        // Verify total items and weights
        Assert.Equal(14, result.TotalItems);
    }

    [Fact]
    public void PlanPacksEmptyItems()
    {
        var emptyItems = new List<Item>();
        var result = _planner.PlanPacks(_config, emptyItems);

        Assert.Equal(1, result.Packs.Count); // Should create one empty pack
        Assert.True(result.Packs[0].IsEmpty);
        Assert.Equal(0, result.TotalItems);
        Assert.Equal(0.0, result.UtilizationPercent);
    }

    [Fact]
    public void PlanPacksLargeQuantities()
    {
        // Create items with large quantities
        var largeItems = new List<Item>
        {
            new Item(1, 100, 50, 1.0),  // 50 items, 50.0 weight
            new Item(2, 200, 30, 2.0)   // 30 items, 60.0 weight
        };

        var result = _planner.PlanPacks(_config, largeItems);

        // Should create multiple packs
        Assert.True(result.Packs.Count >= 8); // At least 8 packs needed

        // Check total items
        Assert.Equal(80, result.TotalItems); // 50+30

        // Verify no pack exceeds constraints
        foreach (var pack in result.Packs)
        {
            if (!pack.IsEmpty)
            {
                Assert.True(pack.TotalItems <= _config.MaxItemsPerPack);
                Assert.True(pack.TotalWeight <= _config.MaxWeightPerPack);
            }
        }
    }

    [Fact]
    public void PlanPacksHeavyItems()
    {
        // Create items that are heavy
        var heavyItems = new List<Item>
        {
            new Item(1, 100, 1, 20.0),  // 1 item, 20.0 weight
            new Item(2, 200, 1, 15.0),  // 1 item, 15.0 weight
            new Item(3, 300, 1, 25.0)   // 1 item, 25.0 weight - exactly at weight limit
        };

        var result = _planner.PlanPacks(_config, heavyItems);

        // Should create 3 packs (one for each item)
        Assert.Equal(3, result.Packs.Count);

        // Check that each pack has one item
        Assert.Equal(1, result.Packs[0].TotalItems);
        Assert.Equal(1, result.Packs[1].TotalItems);
        Assert.Equal(1, result.Packs[2].TotalItems);

        // Check weights
        Assert.Equal(20.0, result.Packs[0].TotalWeight);
        Assert.Equal(15.0, result.Packs[1].TotalWeight);
        Assert.Equal(25.0, result.Packs[2].TotalWeight);
    }

    [Fact]
    public void PlanPacksExtremelyHeavyItem()
    {
        // Create an item that exceeds max weight
        var extremeItems = new List<Item>
        {
            new Item(1, 100, 1, 30.0)  // 1 item, 30.0 weight (exceeds max_weight_per_pack)
        };

        var result = _planner.PlanPacks(_config, extremeItems);

        // Should create many packs but they will be empty because item can't fit
        Assert.True(result.Packs.Count >= 1);
        Assert.Equal(1, result.TotalItems); // Total items count includes all input items
        Assert.Equal(0.0, result.UtilizationPercent);
    }

    [Fact]
    public void CalculateUtilization()
    {
        // Create some packs with known weights
        var testPacks = new List<Pack>();

        var p1 = new Pack(1);
        p1.AddPartialItem(1, 100, 5, 2.0, 10, 25.0); // 10.0 weight

        var p2 = new Pack(2);
        p2.AddPartialItem(2, 200, 3, 5.0, 10, 25.0); // 15.0 weight

        var p3 = new Pack(3); // Empty pack

        testPacks.Add(p1);
        testPacks.Add(p2);
        testPacks.Add(p3);

        double utilization = global::PackPlanner.PackPlanner.CalculateUtilization(testPacks, 25.0);

        // Utilization = (10.0 + 15.0) / (25.0 * 2) * 100 = 50%
        Assert.Equal(50.0, utilization);
    }

    [Fact]
    public void CalculateUtilizationEmptyPacks()
    {
        var emptyPacks = new List<Pack>();
        double utilization = global::PackPlanner.PackPlanner.CalculateUtilization(emptyPacks, 25.0);
        Assert.Equal(0.0, utilization);

        // Add one empty pack
        emptyPacks.Add(new Pack(1));
        utilization = global::PackPlanner.PackPlanner.CalculateUtilization(emptyPacks, 25.0);
        Assert.Equal(0.0, utilization);
    }

    [Fact]
    public void OutputResults()
    {
        // Create a pack with known content
        var p1 = new Pack(1);
        p1.AddPartialItem(1, 100, 5, 2.0, 10, 25.0);

        var testPacks = new List<Pack> { p1 };

        // Capture output
        using var output = new StringWriter();
        global::PackPlanner.PackPlanner.OutputResults(testPacks, output);

        string result = output.ToString();
        Assert.Contains("Pack Number: 1", result);
        Assert.Contains("1,100,5,2.000", result);
        Assert.Contains("Pack Length: 100", result);
        Assert.Contains("Pack Weight: 10.00", result);
    }

    [Fact]
    public void PerformanceTest()
    {
        // Create a large number of items
        var manyItems = new List<Item>();
        var random = new Random(42); // Fixed seed for reproducibility

        for (int i = 0; i < 1000; i++)
        {
            manyItems.Add(new Item(
                i + 1,
                random.Next(50, 500),
                random.Next(1, 5),
                random.NextDouble() * 4.5 + 0.5
            ));
        }

        // Measure time to plan packs
        var stopwatch = Stopwatch.StartNew();
        var result = _planner.PlanPacks(_config, manyItems);
        stopwatch.Stop();

        // Just verify it completes in a reasonable time
        // This is not a strict test, but helps catch major performance regressions
        Assert.True(stopwatch.ElapsedMilliseconds < 3000); // Should complete in less than 3 seconds

        // Verify all items were packed
        int totalPacked = result.Packs.Sum(p => p.TotalItems);
        int totalInput = manyItems.Sum(i => i.Quantity);

        Assert.Equal(totalInput, totalPacked);
    }

    [Fact]
    public void EdgeCaseZeroWeightItems()
    {
        // Create items with zero weight
        var zeroWeightItems = new List<Item>
        {
            new Item(1, 100, 5, 0.0),
            new Item(2, 200, 10, 0.0)
        };

        var result = _planner.PlanPacks(_config, zeroWeightItems);

        // Should be limited by max_items_per_pack
        Assert.Equal(2, result.Packs.Count);
        Assert.Equal(10, result.Packs[0].TotalItems);
        Assert.Equal(5, result.Packs[1].TotalItems);
        Assert.Equal(0.0, result.Packs[0].TotalWeight);
        Assert.Equal(0.0, result.Packs[1].TotalWeight);
    }

    [Fact]
    public void EdgeCaseZeroQuantityItems()
    {
        // Create items with zero quantity
        var zeroQuantityItems = new List<Item>
        {
            new Item(1, 100, 0, 2.0),
            new Item(2, 200, 0, 3.0),
            new Item(3, 300, 5, 1.0)  // One normal item
        };

        var result = _planner.PlanPacks(_config, zeroQuantityItems);

        // Should only pack the normal item
        Assert.Equal(1, result.Packs.Count);
        Assert.Equal(5, result.Packs[0].TotalItems);
        Assert.Equal(5.0, result.Packs[0].TotalWeight);
    }

    [Fact]
    public void EdgeCaseNegativeValues()
    {
        // Create items with negative values (should be handled gracefully)
        var negativeItems = new List<Item>
        {
            new Item(1, -100, 5, 2.0),    // Negative length
            new Item(2, 200, -3, 3.0),    // Negative quantity
            new Item(3, 300, 2, -5.0),    // Negative weight
            new Item(4, 150, 4, 2.5)      // Normal item
        };

        var result = _planner.PlanPacks(_config, negativeItems);

        // Should handle negative values without crashing
        // Exact behavior depends on implementation, but should not crash
        Assert.True(result.Packs.Count >= 1);
    }

    [Fact]
    public void EdgeCaseExtremeValues()
    {
        // Create items with extreme values
        var extremeItems = new List<Item>
        {
            new Item(1, int.MaxValue, 1, 1.0),  // Max int length
            new Item(2, 100, int.MaxValue / 1000, 0.001),  // Very large quantity
            new Item(3, 100, 1, double.Epsilon),  // Min positive double weight
            new Item(4, 100, 1, 1000.0)  // Very large weight
        };

        // This test mainly checks that the code doesn't crash with extreme values
        var result = _planner.PlanPacks(_config, extremeItems);

        // Just verify it completes without crashing
        Assert.True(result.Packs.Count >= 1);
    }
}

/// <summary>
/// Parallel Pack Strategy Tests
/// </summary>
public class ParallelPackStrategyTests
{
    private readonly global::PackPlanner.PackPlanner _planner;
    private readonly List<Item> _items;
    private readonly PackPlannerConfig _config;

    public ParallelPackStrategyTests()
    {
        // Common setup for parallel strategy tests
        _planner = new global::PackPlanner.PackPlanner();

        // Create some test items
        _items = new List<Item>
        {
            new Item(1, 100, 50, 2.0),  // total weight: 100.0
            new Item(2, 200, 30, 3.0),  // total weight: 90.0
            new Item(3, 300, 20, 5.0),  // total weight: 100.0
            new Item(4, 150, 40, 2.5)   // total weight: 100.0
        };

        // Default configuration
        _config = new PackPlannerConfig
        {
            Order = SortOrder.Natural,
            MaxItemsPerPack = 10,
            MaxWeightPerPack = 25.0,
            Type = StrategyType.Parallel,
            ThreadCount = 4
        };
    }

    [Fact]
    public void BasicParallelPacking()
    {
        var result = _planner.PlanPacks(_config, _items);

        // Verify strategy name
        Assert.Contains("Parallel", result.StrategyName);

        // Verify all items were packed
        int totalInput = _items.Sum(i => i.Quantity);

        Assert.Equal(totalInput, result.TotalItems);

        // Verify no pack exceeds constraints
        foreach (var pack in result.Packs)
        {
            if (!pack.IsEmpty)
            {
                Assert.True(pack.TotalItems <= _config.MaxItemsPerPack);
                Assert.True(pack.TotalWeight <= _config.MaxWeightPerPack);
            }
        }
    }

    [Fact]
    public void CompareWithBlockingStrategy()
    {
        // First run with parallel strategy
        var parallelResult = _planner.PlanPacks(_config, _items);

        // Then run with blocking strategy
        var blockingConfig = _config with { Type = StrategyType.Blocking };
        var blockingResult = _planner.PlanPacks(blockingConfig, _items);

        // Both strategies should pack all items
        Assert.Equal(parallelResult.TotalItems, blockingResult.TotalItems);

        // Compare utilization - should be similar (within 5%)
        Assert.True(Math.Abs(parallelResult.UtilizationPercent - blockingResult.UtilizationPercent) <= 5.0);
    }

    [Fact]
    public void ThreadCountImpact()
    {
        // Test with different thread counts
        var threadCounts = new[] { 1, 2, 4, 8 };
        var packingTimes = new List<double>();

        foreach (int threads in threadCounts)
        {
            var config = _config with { ThreadCount = threads };
            var result = _planner.PlanPacks(config, _items);
            packingTimes.Add(result.PackingTime);

            // Verify all items were packed correctly
            Assert.Equal(140, result.TotalItems); // 50+30+20+40
        }

        // This is not a strict test as performance depends on hardware
        // but generally more threads should not be drastically slower
        // Just check that the test runs without errors
        Assert.Equal(threadCounts.Length, packingTimes.Count);
    }
}
