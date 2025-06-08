using Xunit;
using PackPlanner;

namespace PackPlanner.Tests;

/// <summary>
/// Item Class Tests
/// </summary>
public class ItemTests
{
    private readonly Item _basicItem;

    public ItemTests()
    {
        // Common setup for item tests
        _basicItem = new Item(1, 100, 5, 2.5);
    }

    [Fact]
    public void ConstructorAndGetters()
    {
        Assert.Equal(1, _basicItem.Id);
        Assert.Equal(100, _basicItem.Length);
        Assert.Equal(5, _basicItem.Quantity);
        Assert.Equal(2.5, _basicItem.Weight);
    }

    [Fact]
    public void WithQuantity()
    {
        var updatedItem = _basicItem.WithQuantity(10);
        Assert.Equal(10, updatedItem.Quantity);

        // Test with zero
        updatedItem = _basicItem.WithQuantity(0);
        Assert.Equal(0, updatedItem.Quantity);

        // Test with negative (implementation should handle this gracefully)
        updatedItem = _basicItem.WithQuantity(-5);
        Assert.Equal(-5, updatedItem.Quantity);
    }

    [Fact]
    public void TotalWeight()
    {
        Assert.Equal(12.5, _basicItem.TotalWeight); // 5 * 2.5

        // Test with zero quantity
        var zeroQuantityItem = _basicItem.WithQuantity(0);
        Assert.Equal(0.0, zeroQuantityItem.TotalWeight);

        // Test with large values
        var largeQuantityItem = _basicItem.WithQuantity(1000000);
        Assert.Equal(1000000 * 2.5, largeQuantityItem.TotalWeight);
    }

    [Fact]
    public void ToString_Test()
    {
        string expected = "1,100,5,2.500";
        Assert.Equal(expected, _basicItem.ToString());

        // Test with different values
        var zeroItem = new Item(0, 0, 0, 0.0);
        Assert.Equal("0,0,0,0.000", zeroItem.ToString());

        // Test with negative values
        var negativeItem = new Item(-1, -100, -5, -2.5);
        Assert.Equal("-1,-100,-5,-2.500", negativeItem.ToString());
    }

    [Fact]
    public void ComparisonOperators()
    {
        var shorter = new Item(2, 50, 5, 2.5);
        var longer = new Item(3, 150, 5, 2.5);
        var sameLength = new Item(4, 100, 10, 5.0);

        // Less than operator (compares length)
        Assert.True(shorter < _basicItem);
        Assert.False(_basicItem < shorter);
        Assert.True(_basicItem < longer);
        Assert.False(longer < _basicItem);
        Assert.False(_basicItem < sameLength);
        Assert.False(sameLength < _basicItem);

        // Greater than operator (compares length)
        Assert.False(shorter > _basicItem);
        Assert.True(_basicItem > shorter);
        Assert.False(_basicItem > longer);
        Assert.True(longer > _basicItem);
        Assert.False(_basicItem > sameLength);
        Assert.False(sameLength > _basicItem);
    }

    [Fact]
    public void CompareTo()
    {
        var shorter = new Item(2, 50, 5, 2.5);
        var longer = new Item(3, 150, 5, 2.5);
        var sameLength = new Item(4, 100, 10, 5.0);

        Assert.True(_basicItem.CompareTo(shorter) > 0);
        Assert.True(_basicItem.CompareTo(longer) < 0);
        Assert.Equal(0, _basicItem.CompareTo(sameLength));
    }
}
