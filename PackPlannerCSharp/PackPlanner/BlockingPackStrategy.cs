namespace PackPlanner;

/// <summary>
/// Blocking (synchronous) pack strategy
/// Sequential processing of items one by one
/// </summary>
public class BlockingPackStrategy : IPackStrategy
{
    /// <summary>
    /// Gets the strategy name
    /// </summary>
    public string Name => "Blocking";

    /// <summary>
    /// Pack items into packs sequentially
    /// </summary>
    /// <param name="items">Items to pack</param>
    /// <param name="maxItems">Maximum items per pack</param>
    /// <param name="maxWeight">Maximum weight per pack</param>
    /// <returns>List of packs</returns>
    public List<Pack> PackItems(IReadOnlyList<Item> items, int maxItems, double maxWeight)
    {
        var packs = new List<Pack>();
        
        // Pre-allocate based on empirical ratio to avoid reallocations
        int estimatedPacks = Math.Max(64, (int)(items.Count * 0.00222) + 16);
        packs.Capacity = estimatedPacks;
        
        int packNumber = 1;
        packs.Add(new Pack(packNumber));

        foreach (var item in items)
        {
            int remainingQuantity = item.Quantity;

            while (remainingQuantity > 0)
            {
                var currentPack = packs[^1]; // Get last pack (C# 8.0+ syntax)
                int addedQuantity = currentPack.AddPartialItem(
                    item.Id, item.Length, remainingQuantity,
                    item.Weight, maxItems, maxWeight);

                if (addedQuantity > 0)
                {
                    remainingQuantity -= addedQuantity;
                }
                else
                {
                    packs.Add(new Pack(++packNumber));
                }
            }
        }

        return packs;
    }
}
