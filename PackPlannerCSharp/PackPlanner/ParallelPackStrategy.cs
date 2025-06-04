using System.Collections.Concurrent;

namespace PackPlanner;

/// <summary>
/// Parallel pack strategy using multiple threads
/// Divides items into chunks and processes them in parallel
/// </summary>
public class ParallelPackStrategy : IPackStrategy
{
    private readonly int _numThreads;

    /// <summary>
    /// Gets the strategy name
    /// </summary>
    public string Name => $"Parallel({_numThreads} threads)";

    /// <summary>
    /// Initializes a new instance of the ParallelPackStrategy class
    /// </summary>
    /// <param name="threadCount">Number of threads to use (0 = use processor count)</param>
    public ParallelPackStrategy(int threadCount = 4)
    {
        _numThreads = threadCount == 0 ? Environment.ProcessorCount : threadCount;
    }

    /// <summary>
    /// Pack items into packs using multiple threads
    /// </summary>
    /// <param name="items">Items to pack</param>
    /// <param name="maxItems">Maximum items per pack</param>
    /// <param name="maxWeight">Maximum weight per pack</param>
    /// <returns>List of packs</returns>
    public List<Pack> PackItems(IReadOnlyList<Item> items, int maxItems, double maxWeight)
    {
        // If items are few or we have only 1 thread, use sequential approach
        // Hybrid approach for better performance on small datasets
        if (items.Count < 5000 || _numThreads == 1)
        {
            return PackItemsSequential(items, maxItems, maxWeight);
        }

        // For parallel processing
        var resultPacks = new ConcurrentBag<Pack>();
        var nextPackNumber = 1;
        var packNumberLock = new object();

        // Calculate chunk size for each thread
        int chunkSize = items.Count / _numThreads;
        int remainder = items.Count % _numThreads;

        // Create and start tasks
        var tasks = new Task[_numThreads];
        int startIdx = 0;

        for (int i = 0; i < _numThreads; i++)
        {
            // Distribute remainder items among first 'remainder' threads
            int threadChunkSize = chunkSize + (i < remainder ? 1 : 0);
            int endIdx = startIdx + threadChunkSize;

            int threadStartIdx = startIdx; // Capture for closure
            int threadEndIdx = endIdx;

            tasks[i] = Task.Run(() => WorkerThread(
                items, threadStartIdx, threadEndIdx,
                maxItems, maxWeight, resultPacks,
                ref nextPackNumber, packNumberLock));

            startIdx = endIdx;
        }

        // Wait for all tasks to complete
        Task.WaitAll(tasks);

        // Convert ConcurrentBag to List and sort by pack number
        var finalPacks = resultPacks.ToList();
        finalPacks.Sort((a, b) => a.PackNumber.CompareTo(b.PackNumber));

        return finalPacks;
    }

    /// <summary>
    /// Sequential packing for small datasets or single thread
    /// </summary>
    /// <param name="items">Items to pack</param>
    /// <param name="maxItems">Maximum items per pack</param>
    /// <param name="maxWeight">Maximum weight per pack</param>
    /// <returns>List of packs</returns>
    private static List<Pack> PackItemsSequential(IReadOnlyList<Item> items, int maxItems, double maxWeight)
    {
        var packs = new List<Pack>();
        int estimatedPacks = Math.Max(64, (int)(items.Count * 0.00222) + 16);
        packs.Capacity = estimatedPacks;

        int packNumber = 1;
        packs.Add(new Pack(packNumber));

        foreach (var item in items)
        {
            int remainingQuantity = item.Quantity;

            while (remainingQuantity > 0)
            {
                var currentPack = packs[^1];
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

    /// <summary>
    /// Worker method for a thread to process a chunk of items
    /// </summary>
    /// <param name="items">Items to process</param>
    /// <param name="startIdx">Starting index in the items collection</param>
    /// <param name="endIdx">Ending index in the items collection</param>
    /// <param name="maxItems">Maximum items per pack</param>
    /// <param name="maxWeight">Maximum weight per pack</param>
    /// <param name="resultPacks">Thread-safe collection to store resulting packs</param>
    /// <param name="nextPackNumber">Reference to the next pack number counter</param>
    /// <param name="packNumberLock">Lock object for thread synchronization</param>
    private static void WorkerThread(
        IReadOnlyList<Item> items,
        int startIdx,
        int endIdx,
        int maxItems,
        double maxWeight,
        ConcurrentBag<Pack> resultPacks,
        ref int nextPackNumber,
        object packNumberLock)
    {
        // Process items in this thread's chunk
        var localPacks = new List<Pack>();
        int estimatedLocalPacks = Math.Max(16, (int)((endIdx - startIdx) * 0.00222) + 8);
        localPacks.Capacity = estimatedLocalPacks;

        // Get first pack number for this thread
        int packNumber;
        lock (packNumberLock)
        {
            packNumber = nextPackNumber++;
        }
        localPacks.Add(new Pack(packNumber));

        for (int i = startIdx; i < endIdx; i++)
        {
            var item = items[i];
            int remainingQuantity = item.Quantity;

            while (remainingQuantity > 0)
            {
                var currentPack = localPacks[^1];
                int addedQuantity = currentPack.AddPartialItem(
                    item.Id,
                    item.Length,
                    remainingQuantity,
                    item.Weight,
                    maxItems,
                    maxWeight);

                if (addedQuantity > 0)
                {
                    remainingQuantity -= addedQuantity;
                }
                else
                {
                    // Get next pack number atomically
                    lock (packNumberLock)
                    {
                        packNumber = nextPackNumber++;
                    }
                    localPacks.Add(new Pack(packNumber));
                }
            }
        }

        // Add local results to the shared result collection
        foreach (var pack in localPacks)
        {
            resultPacks.Add(pack);
        }
    }
}
