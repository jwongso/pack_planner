using PackPlanner;

namespace PackPlanner.Tests;

public class BlockingStrategyTests : PackPlannerStrategyTestBase
{
    public BlockingStrategyTests() : base(StrategyType.Blocking) { }

    // --- Correctness Tests ---
    [Fact]
    public void PlanPacksNaturalOrder_CreatesValidPacks()
    {
        var result = _planner.PlanPacks(_config, _items);

        Assert.Equal(2, result.Packs.Count);
        Assert.Equal(9, result.Packs[0].TotalItems);
        Assert.Equal(24.0, result.Packs[0].TotalWeight);
        AssertPacksMeetConstraints(result);
    }

    [Theory]
    [InlineData(SortOrder.Natural)]
    [InlineData(SortOrder.ShortToLong)]
    [InlineData(SortOrder.LongToShort)]
    public void PlanPacks_RespectsSortOrder(SortOrder order)
    {
        _config = _config with { Order = order };
        var result = _planner.PlanPacks(_config, _items);

        // Verify first item in first pack matches sort order
        var firstItem = result.Packs[0].Items[0];
        switch (order)
        {
            case SortOrder.ShortToLong:
                Assert.Equal(100, firstItem.Length);
                break;
            case SortOrder.LongToShort:
                Assert.Equal(300, firstItem.Length);
                break;
        }
    }

    // --- Edge Cases ---
    [Fact]
    public void PlanPacks_EmptyInput_ReturnsEmptyPack()
    {
        var result = _planner.PlanPacks(_config, new List<Item>());
        Assert.Single(result.Packs);
        Assert.True(result.Packs[0].IsEmpty);
    }

    [Fact]
    public void PlanPacks_ItemExceedsMaxWeight_ReturnsEmptyPack()
    {
        var heavyItem = new List<Item> { new Item(1, 100, 1, 30.0) }; // Exceeds 25.0 limit
        var result = _planner.PlanPacks(_config, heavyItem);

        Assert.Single(result.Packs);
        Assert.Equal(0.0, result.UtilizationPercent);
    }

    [Fact]
    public void PlanPacksNaturalOrderBlocking()
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
        Assert.True(Math.Abs(result.UtilizationPercent - 78.0) < 0.1);
    }

    [Fact]
    public void PlanPacksShortToLongBlocking()
    {
        _config = _config with { Order = SortOrder.ShortToLong };
        var result = _planner.PlanPacks(_config, _items);

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
    public void PlanPacksLongToShortBlocking()
    {
        _config = _config with { Order = SortOrder.LongToShort };
        var result = _planner.PlanPacks(_config, _items);

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
    public void PlanPacksEmptyItemsBlocking()
    {
        var emptyItems = new List<Item>();
        var result = _planner.PlanPacks(_config, emptyItems);

        Assert.Single(result.Packs); // Should create one empty pack
        Assert.True(result.Packs[0].IsEmpty);
        Assert.Equal(0, result.TotalItems);
        Assert.Equal(0.0, result.UtilizationPercent);
    }

    [Fact]
    public void PlanPacksLargeQuantitiesBlocking()
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
    public void PlanPacksHeavyItemsBlocking()
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
    public void PlanPacksExtremelyHeavyItemBlocking()
    {
        // Create an item that exceeds max weight
        var extremeItems = new List<Item>
        {
            new Item(1, 100, 1, 30.0)  // 1 item, 30.0 weight (exceeds max_weight_per_pack)
        };

        var result = _planner.PlanPacks(_config, extremeItems);

        // Should create 1 pack but it will be empty because item can't fit
        Assert.True(result.Packs.Count >= 1);
        Assert.Equal(1, result.TotalItems); // Total items count includes all input items
        Assert.Equal(0.0, result.UtilizationPercent);
    }

    [Fact]
    public void EdgeCaseZeroWeightItemsBlocking()
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
    public void EdgeCaseZeroQuantityItemsBlocking()
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
        Assert.Single(result.Packs);
        Assert.Equal(5, result.Packs[0].TotalItems);
        Assert.Equal(5.0, result.Packs[0].TotalWeight);
    }

    [Fact]
    public void EdgeCaseNegativeValuesBlocking()
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
}
