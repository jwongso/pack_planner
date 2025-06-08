using System.Text;

namespace PackPlanner;

/// <summary>
/// Represents a pack containing multiple items with constraint checking
/// </summary>
public class Pack
{
    private readonly List<Item> _items;
    private int _totalItems;
    private double _totalWeight;
    private int _maxLength;

    /// <summary>
    /// The pack identifier
    /// </summary>
    public int PackNumber { get; }

    /// <summary>
    /// Gets the items in the pack
    /// </summary>
    public IReadOnlyList<Item> Items => _items;

    /// <summary>
    /// Gets the total number of items in the pack
    /// </summary>
    public int TotalItems => _totalItems;

    /// <summary>
    /// Gets the total weight of the pack
    /// </summary>
    public double TotalWeight => _totalWeight;

    /// <summary>
    /// Gets the maximum length of any item in the pack
    /// </summary>
    public int PackLength => _maxLength;

    /// <summary>
    /// Checks if the pack is empty
    /// </summary>
    public bool IsEmpty => _items.Count == 0;

    /// <summary>
    /// Initializes a new instance of the Pack class
    /// </summary>
    /// <param name="packNumber">The pack identifier</param>
    public Pack(int packNumber)
    {
        PackNumber = packNumber;
        _items = new List<Item>(8); // Reserve some space to avoid initial reallocations
        _totalItems = 0;
        _totalWeight = 0.0;
        _maxLength = 0;
    }

    /// <summary>
    /// Add item to pack
    /// </summary>
    /// <param name="item">The item to add</param>
    /// <param name="maxItems">Maximum number of items allowed in the pack</param>
    /// <param name="maxWeight">Maximum weight allowed in the pack</param>
    /// <returns>True if successful, false if constraints violated</returns>
    public bool AddItem(Item item, int maxItems, double maxWeight)
    {
        int newQuantity = _totalItems + item.Quantity;
        double newWeight = _totalWeight + item.TotalWeight;

        if (newQuantity <= maxItems && newWeight <= maxWeight)
        {
            _items.Add(item);
            _totalItems = newQuantity;
            _totalWeight = newWeight;
            _maxLength = Math.Max(_maxLength, item.Length);
            return true;
        }
        return false;
    }

    /// <summary>
    /// Try to add partial quantity of an item (optimized version)
    /// </summary>
    /// <param name="item">The item to add</param>
    /// <param name="remainingQuantity">Remaining quantity of the item</param>
    /// <param name="maxItems">Maximum number of items allowed in the pack</param>
    /// <param name="maxWeight">Maximum weight allowed in the pack</param>
    /// <returns>Number of items successfully added</returns>
    public int AddPartialItem(Item item, int remainingQuantity, int maxItems, double maxWeight)
    {
        // Early exit if pack constraints are already exceeded
        if (_totalItems >= maxItems || _totalWeight >= maxWeight)
        {
            return 0;
        }

        int maxByItems = maxItems - _totalItems;

        // Only calculate weight constraint if we haven't already hit the item limit
        int canAdd = Math.Min(maxByItems, remainingQuantity);
        if (canAdd > 0)
        {
            // Calculate how many we can fit by weight
            double remainingWeight = maxWeight - _totalWeight;
            int maxByWeight = (int)(remainingWeight / item.Weight);
            canAdd = Math.Min(canAdd, maxByWeight);
        }

        if (canAdd > 0)
        {
            var partialItem = new Item(item.Id, item.Length, canAdd, item.Weight);
            _items.Add(partialItem);
            _totalItems += canAdd;
            _totalWeight += canAdd * item.Weight;
            _maxLength = Math.Max(_maxLength, item.Length);
        }

        return canAdd;
    }

    /// <summary>
    /// Try to add partial quantity of an item (backward compatibility)
    /// </summary>
    /// <param name="item">The item to add</param>
    /// <param name="maxItems">Maximum number of items allowed in the pack</param>
    /// <param name="maxWeight">Maximum weight allowed in the pack</param>
    /// <returns>Number of items successfully added</returns>
    public int AddPartialItem(Item item, int maxItems, double maxWeight)
    {
        return AddPartialItem(item, item.Quantity, maxItems, maxWeight);
    }

    /// <summary>
    /// Try to add partial quantity of an item using individual parameters
    /// </summary>
    /// <param name="id">The item ID</param>
    /// <param name="length">The item length</param>
    /// <param name="quantity">The item quantity</param>
    /// <param name="weight">The item weight per piece</param>
    /// <param name="maxItems">Maximum number of items allowed in the pack</param>
    /// <param name="maxWeight">Maximum weight allowed in the pack</param>
    /// <returns>Number of items successfully added</returns>
    public int AddPartialItem(int id, int length, int quantity, double weight,
                             int maxItems, double maxWeight)
    {
        // SAFETY: Validate inputs to prevent negative values
        if (quantity <= 0 || maxItems <= 0 || maxWeight < 0)
        {
            return 0;
        }

        // SAFETY: Ensure length is positive for valid packing
        length = Math.Max(1, length);

        // SAFETY: Ensure weight is non-negative
        weight = Math.Max(0.0, weight);

        int maxByItems = maxItems - _totalItems;
        double weightRemaining = maxWeight - _totalWeight;

        // Handle zero weight case - if weight is 0, weight constraint doesn't apply
        int maxByWeight = (weight == 0.0) ? quantity : (int)(weightRemaining / weight);

        // SAFETY: Ensure maxByWeight is non-negative to prevent underflow
        int safeMaxByWeight = Math.Max(0, maxByWeight);

        int canAdd = Math.Min(Math.Min(maxByItems, safeMaxByWeight), quantity);

        if (canAdd > 0)
        {
            var item = new Item(id, length, canAdd, weight);
            _items.Add(item);
            _totalItems += canAdd;
            _totalWeight += canAdd * weight;
            _maxLength = Math.Max(_maxLength, length);
        }
        return canAdd;
    }

    /// <summary>
    /// Check if the pack is full
    /// </summary>
    /// <param name="maxItems">Maximum number of items allowed in the pack</param>
    /// <param name="maxWeight">Maximum weight allowed in the pack</param>
    /// <returns>True if the pack is full</returns>
    public bool IsFull(int maxItems, double maxWeight)
    {
        // Floating-point precision fix by epsilon
        return _totalItems >= maxItems || _totalWeight >= maxWeight - 1e-9;
    }

    /// <summary>
    /// Gets string representation of the pack
    /// </summary>
    /// <returns>The string representation</returns>
    public override string ToString()
    {
        var sb = new StringBuilder();
        sb.AppendLine($"Pack Number: {PackNumber}");

        foreach (var item in _items)
        {
            sb.AppendLine(item.ToString());
        }

        sb.Append($"Pack Length: {PackLength}, Pack Weight: {TotalWeight:F2}");

        return sb.ToString();
    }
}
