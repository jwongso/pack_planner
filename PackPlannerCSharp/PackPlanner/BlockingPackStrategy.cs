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
        // SAFETY: Validate constraints to prevent infinite loops
        maxItems = Math.Max(1, maxItems);
        maxWeight = Math.Max(0.1, maxWeight);

        var packs = new List<Pack>();
        
        // Pre-allocate based on empirical ratio to avoid reallocations
        // SAFETY: Limit initial allocation to prevent OOM with extreme values
        int maxSafeReserve = Math.Min(100000, items.Count / 10 + 1000);
        int estimatedPacks = Math.Max(64, (int)(items.Count * 0.00222) + 16);
        packs.Capacity = Math.Min(maxSafeReserve, estimatedPacks);
        
        int packNumber = 1;
        packs.Add(new Pack(packNumber));

        // SAFETY: Add a safety counter to prevent infinite loops
        const int maxIterations = 1000000; // Reasonable upper limit
        int safetyCounter = 0;

        foreach (var item in items)
        {
            // SAFETY: Skip items with non-positive quantities
            if (item.Quantity <= 0) continue;

            int remainingQuantity = item.Quantity;

            while (remainingQuantity > 0)
            {
                // SAFETY: Check for potential infinite loop
                if (++safetyCounter > maxIterations)
                {
                    // Force exit the loop if we've exceeded reasonable iterations
                    break;
                }

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
                    // SAFETY: Limit maximum number of packs to prevent OOM
                    if (packs.Count >= maxSafeReserve)
                    {
                        // Force exit if we've created too many packs
                        remainingQuantity = 0;
                        break;
                    }
                    packs.Add(new Pack(++packNumber));
                }
            }
        }

        return packs;
    }
}
