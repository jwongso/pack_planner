using Xunit;
using PackPlanner;

namespace PackPlanner.Tests;

/// <summary>
/// Pack Class Tests
/// </summary>
public class PackTests
{
    private readonly Pack _pack1;
    private readonly Item _item1;
    private readonly Item _item2;
    private readonly Item _item3;
    private const int DefaultMaxItems = 20;
    private const double DefaultMaxWeight = 50.0;

    public PackTests()
    {
        // Common setup for pack tests
        _pack1 = new Pack(1);
        _item1 = new Item(1, 100, 5, 2.0);  // total weight: 10.0
        _item2 = new Item(2, 200, 3, 3.0);  // total weight: 9.0
        _item3 = new Item(3, 300, 2, 5.0);  // total weight: 10.0
    }

    [Fact]
    public void Constructor()
    {
        Assert.Equal(1, _pack1.PackNumber);
        Assert.True(_pack1.IsEmpty);
        Assert.Equal(0, _pack1.TotalItems);
        Assert.Equal(0.0, _pack1.TotalWeight);
        Assert.Equal(0, _pack1.PackLength);
    }

    [Fact]
    public void AddItem()
    {
        // Add first item
        Assert.True(_pack1.AddItem(_item1, DefaultMaxItems, DefaultMaxWeight));
        Assert.False(_pack1.IsEmpty);
        Assert.Equal(5, _pack1.TotalItems);
        Assert.Equal(10.0, _pack1.TotalWeight);
        Assert.Equal(100, _pack1.PackLength);

        // Add second item
        Assert.True(_pack1.AddItem(_item2, DefaultMaxItems, DefaultMaxWeight));
        Assert.Equal(8, _pack1.TotalItems);
        Assert.Equal(19.0, _pack1.TotalWeight);
        Assert.Equal(200, _pack1.PackLength);

        // Add third item
        Assert.True(_pack1.AddItem(_item3, DefaultMaxItems, DefaultMaxWeight));
        Assert.Equal(10, _pack1.TotalItems);
        Assert.Equal(29.0, _pack1.TotalWeight);
        Assert.Equal(300, _pack1.PackLength);
    }

    [Fact]
    public void AddItemExceedingMaxItems()
    {
        // Create an item with quantity that exceeds max_items
        var largeQuantity = new Item(4, 100, 25, 1.0);

        // Should fail because it exceeds max_items
        Assert.False(_pack1.AddItem(largeQuantity, DefaultMaxItems, DefaultMaxWeight));
        Assert.True(_pack1.IsEmpty);

        // Add some items first
        Assert.True(_pack1.AddItem(_item1, DefaultMaxItems, DefaultMaxWeight)); // 5 items

        // Try to add an item that would exceed max_items
        var anotherLarge = new Item(5, 100, 16, 1.0);
        Assert.False(_pack1.AddItem(anotherLarge, DefaultMaxItems, DefaultMaxWeight));
        Assert.Equal(5, _pack1.TotalItems); // Should remain unchanged
    }

    [Fact]
    public void AddItemExceedingMaxWeight()
    {
        // Create an item with weight that exceeds max_weight
        var heavyItem = new Item(4, 100, 1, 60.0);

        // Should fail because it exceeds max_weight
        Assert.False(_pack1.AddItem(heavyItem, DefaultMaxItems, DefaultMaxWeight));
        Assert.True(_pack1.IsEmpty);

        // Add some items first
        Assert.True(_pack1.AddItem(_item1, DefaultMaxItems, DefaultMaxWeight)); // 10.0 weight

        // Try to add an item that would exceed max_weight
        var anotherHeavy = new Item(5, 100, 5, 10.0); // 50.0 weight
        Assert.False(_pack1.AddItem(anotherHeavy, DefaultMaxItems, DefaultMaxWeight));
        Assert.Equal(10.0, _pack1.TotalWeight); // Should remain unchanged
    }

    [Fact]
    public void AddPartialItem()
    {
        // Test adding partial quantity
        int added = _pack1.AddPartialItem(_item1, DefaultMaxItems, DefaultMaxWeight);
        Assert.Equal(5, added); // Should add all 5
        Assert.Equal(5, _pack1.TotalItems);

        // Test adding partial quantity with remaining_quantity parameter
        var largeQuantity = new Item(4, 100, 30, 1.0);
        added = _pack1.AddPartialItem(largeQuantity, DefaultMaxItems, DefaultMaxWeight);
        Assert.Equal(15, added); // Should add only 15 out of requested 30
        Assert.Equal(20, _pack1.TotalItems);

        // Test adding partial quantity with individual parameters
        added = _pack1.AddPartialItem(5, 150, 10, 2.0, DefaultMaxItems, DefaultMaxWeight);
        Assert.Equal(0, added); // Should add 0 as we're at max_items
        Assert.Equal(20, _pack1.TotalItems);
        Assert.Equal(100, _pack1.PackLength);
    }

    [Fact]
    public void AddPartialItemWeightConstraint()
    {
        // Fill pack to near weight limit
        Assert.True(_pack1.AddItem(_item1, DefaultMaxItems, DefaultMaxWeight)); // 10.0 weight
        Assert.True(_pack1.AddItem(_item2, DefaultMaxItems, DefaultMaxWeight)); // +9.0 = 19.0 weight
        Assert.True(_pack1.AddItem(_item3, DefaultMaxItems, DefaultMaxWeight)); // +10.0 = 29.0 weight

        // Try to add an item that would partially fit by weight
        var heavyItem = new Item(4, 100, 10, 3.0); // Each piece is 3.0 weight
        int added = _pack1.AddPartialItem(heavyItem, DefaultMaxItems, DefaultMaxWeight);

        // Should only add 7 pieces (7*3.0 = 21.0, bringing total to 50.0)
        Assert.Equal(7, added);
        Assert.Equal(17, _pack1.TotalItems); // 5+3+2+7
        Assert.Equal(50.0, _pack1.TotalWeight);
    }

    [Fact]
    public void AddPartialItemZeroCase()
    {
        // Fill pack to max items
        var smallItem = new Item(4, 50, DefaultMaxItems, 1.0);
        _pack1.AddPartialItem(smallItem, DefaultMaxItems, DefaultMaxWeight);

        // Try to add more items when already full
        int added = _pack1.AddPartialItem(_item1, DefaultMaxItems, DefaultMaxWeight);
        Assert.Equal(0, added);

        // Create a new pack for weight test
        var pack2 = new Pack(2);

        // Fill pack to max weight
        var weightFiller = new Item(5, 50, 5, 10.0); // 50.0 weight total
        added = pack2.AddPartialItem(weightFiller, DefaultMaxItems, DefaultMaxWeight);

        // Try to add more items when already at weight limit
        added = pack2.AddPartialItem(_item1, DefaultMaxItems, DefaultMaxWeight);
        Assert.Equal(0, added);
    }

    [Fact]
    public void IsFull()
    {
        Assert.False(_pack1.IsFull(DefaultMaxItems, DefaultMaxWeight));

        // Fill to max items
        var filler = new Item(4, 50, DefaultMaxItems, 1.0);
        int added = _pack1.AddPartialItem(filler, DefaultMaxItems, DefaultMaxWeight);
        Assert.True(_pack1.IsFull(DefaultMaxItems, DefaultMaxWeight));

        // Test with weight
        var pack2 = new Pack(2);
        Assert.False(pack2.IsFull(DefaultMaxItems, DefaultMaxWeight));

        // Fill to max weight
        var weightFiller = new Item(5, 50, 5, 10.0); // 50.0 weight total
        added = pack2.AddPartialItem(weightFiller, DefaultMaxItems, DefaultMaxWeight);
        Assert.True(pack2.IsFull(DefaultMaxItems, DefaultMaxWeight));

        // Test with almost full (weight)
        var pack3 = new Pack(3);
        var almostFull = new Item(6, 50, 1, 49.99);
        Assert.True(pack3.AddItem(almostFull, DefaultMaxItems, DefaultMaxWeight));
        Assert.False(pack3.IsFull(DefaultMaxItems, DefaultMaxWeight)); // Should not be considered full due to epsilon
    }

    [Fact]
    public void GetItems()
    {
        Assert.True(_pack1.AddItem(_item1, DefaultMaxItems, DefaultMaxWeight));
        Assert.True(_pack1.AddItem(_item2, DefaultMaxItems, DefaultMaxWeight));

        var items = _pack1.Items;
        Assert.Equal(2, items.Count);
        Assert.Equal(1, items[0].Id);
        Assert.Equal(2, items[1].Id);
    }

    [Fact]
    public void ToString_Test()
    {
        Assert.True(_pack1.AddItem(_item1, DefaultMaxItems, DefaultMaxWeight));

        string result = _pack1.ToString();
        Assert.Contains("Pack Number: 1", result);
        Assert.Contains("1,100,5,2.000", result);
        Assert.Contains("Pack Length: 100", result);
        Assert.Contains("Pack Weight: 10.00", result);
    }
}
