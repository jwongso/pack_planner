using System.Globalization;

namespace PackPlanner;

/// <summary>
/// Represents an item with id, length, quantity, and weight properties.
/// This class is immutable for thread safety and performance.
/// </summary>
public readonly record struct Item
{
    /// <summary>
    /// The item identifier
    /// </summary>
    public int Id { get; }

    /// <summary>
    /// The item length
    /// </summary>
    public int Length { get; }

    /// <summary>
    /// The item quantity
    /// </summary>
    public int Quantity { get; }

    /// <summary>
    /// The weight per piece
    /// </summary>
    public double Weight { get; }

    /// <summary>
    /// Initializes a new instance of the Item struct
    /// </summary>
    /// <param name="id">The item identifier</param>
    /// <param name="length">The item length</param>
    /// <param name="quantity">The item quantity</param>
    /// <param name="weight">The weight per piece</param>
    public Item(int id, int length, int quantity, double weight)
    {
        Id = id;
        Length = length;
        Quantity = quantity;
        Weight = weight;
    }

    /// <summary>
    /// Gets the total weight for this item (quantity * weight per piece)
    /// </summary>
    public double TotalWeight => Quantity * Weight;

    /// <summary>
    /// Creates a new Item with the specified quantity
    /// </summary>
    /// <param name="quantity">The new quantity</param>
    /// <returns>A new Item with the updated quantity</returns>
    public Item WithQuantity(int quantity) => new(Id, Length, quantity, Weight);

    /// <summary>
    /// Gets string representation of the item
    /// </summary>
    /// <returns>The string representation</returns>
    public override string ToString()
    {
        return $"{Id},{Length},{Quantity},{Weight:F3}";
    }

    /// <summary>
    /// Comparison for sorting by length (short to long)
    /// </summary>
    /// <param name="other">The other item to compare with</param>
    /// <returns>Comparison result</returns>
    public int CompareTo(Item other)
    {
        return Length.CompareTo(other.Length);
    }

    /// <summary>
    /// Implements IComparable for sorting support
    /// </summary>
    public static bool operator <(Item left, Item right) => left.Length < right.Length;
    public static bool operator >(Item left, Item right) => left.Length > right.Length;
    public static bool operator <=(Item left, Item right) => left.Length <= right.Length;
    public static bool operator >=(Item left, Item right) => left.Length >= right.Length;
}
