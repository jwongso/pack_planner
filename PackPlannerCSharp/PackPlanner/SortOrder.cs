namespace PackPlanner;

/// <summary>
/// Enumeration for different item sorting orders
/// </summary>
public enum SortOrder
{
    /// <summary>
    /// Keep original order
    /// </summary>
    Natural,

    /// <summary>
    /// Sort by length ascending (short to long)
    /// </summary>
    ShortToLong,

    /// <summary>
    /// Sort by length descending (long to short)
    /// </summary>
    LongToShort
}

/// <summary>
/// Extension methods for SortOrder enum
/// </summary>
public static class SortOrderExtensions
{
    /// <summary>
    /// Parse a string to get the corresponding SortOrder
    /// </summary>
    /// <param name="str">The string to parse</param>
    /// <returns>The parsed sort order</returns>
    public static SortOrder ParseSortOrder(string str)
    {
        return str.ToUpperInvariant() switch
        {
            "NATURAL" => SortOrder.Natural,
            "SHORT_TO_LONG" => SortOrder.ShortToLong,
            "LONG_TO_SHORT" => SortOrder.LongToShort,
            _ => SortOrder.Natural // default
        };
    }

    /// <summary>
    /// Convert a SortOrder to its string representation
    /// </summary>
    /// <param name="order">The sort order to convert</param>
    /// <returns>The string representation</returns>
    public static string ToShortString(this SortOrder order)
    {
        return order switch
        {
            SortOrder.Natural => "NAT",
            SortOrder.ShortToLong => "STL",
            SortOrder.LongToShort => "LTS",
            _ => "NAT"
        };
    }

    /// <summary>
    /// Sort items according to the specified order
    /// </summary>
    /// <param name="items">Items to sort</param>
    /// <param name="order">Sort order to use</param>
    public static void SortItems(List<Item> items, SortOrder order)
    {
        switch (order)
        {
            case SortOrder.ShortToLong:
                items.Sort((a, b) => a.Length.CompareTo(b.Length));
                break;
            case SortOrder.LongToShort:
                items.Sort((a, b) => b.Length.CompareTo(a.Length));
                break;
            case SortOrder.Natural:
            default:
                // Keep original order
                break;
        }
    }
}
