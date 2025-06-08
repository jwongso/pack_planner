using PackPlanner;
using System.Globalization;

namespace PackPlanner;

/// <summary>
/// Main program entry point
/// </summary>
public class Program
{
    /// <summary>
    /// Main entry point
    /// </summary>
    /// <param name="args">Command line arguments</param>
    /// <returns>Exit code</returns>
    public static int Main(string[] args)
    {
        // Check for benchmark mode
        if (args.Length == 1 && args[0] == "--benchmark")
        {
            var benchmark = new Benchmark();
            benchmark.RunBenchmark();
            return 0;
        }

        // Check for help
        if (args.Length == 1 && (args[0] == "--help" || args[0] == "-h"))
        {
            PrintUsage("PackPlanner");
            return 0;
        }

        var planner = new PackPlanner();
        var config = new PackPlannerConfig();
        var items = new List<Item>();

        bool parseSuccess = false;

        if (args.Length == 0)
        {
            // Read from standard input
            parseSuccess = ParseInput(Console.In, ref config, items);
        }
        else if (args.Length == 1)
        {
            // Read from file
            try
            {
                using var inputFile = new StreamReader(args[0]);
                parseSuccess = ParseInput(inputFile, ref config, items);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"Error: Could not open input file: {args[0]} - {ex.Message}");
                return 1;
            }
        }
        else
        {
            Console.Error.WriteLine("Error: Invalid number of arguments.");
            PrintUsage("PackPlanner");
            return 1;
        }

        if (!parseSuccess)
        {
            Console.Error.WriteLine("Error: Failed to parse input.");
            return 1;
        }

        if (items.Count == 0)
        {
            Console.Error.WriteLine("Error: No items to pack.");
            return 1;
        }

        // Plan packs
        var result = planner.PlanPacks(config, items);

        // Output results
        PackPlanner.OutputResults(result.Packs);

        return 0;
    }

    /// <summary>
    /// Print usage information
    /// </summary>
    /// <param name="programName">Name of the program</param>
    private static void PrintUsage(string programName)
    {
        Console.WriteLine("Usage:");
        Console.WriteLine($"  {programName}                    - Read from standard input");
        Console.WriteLine($"  {programName} <input_file>       - Read from input file");
        Console.WriteLine($"  {programName} --benchmark        - Run performance benchmark");
    }

    /// <summary>
    /// Parse input from stream into configuration and items
    /// </summary>
    /// <param name="input">Input stream to read from</param>
    /// <param name="config">Configuration to populate</param>
    /// <param name="items">List to populate with items</param>
    /// <returns>True if parsing was successful</returns>
    private static bool ParseInput(TextReader input, ref PackPlannerConfig config, List<Item> items)
    {
        string? line = input.ReadLine();

        // Parse first line: sort order, max items, max weight
        if (string.IsNullOrEmpty(line))
        {
            return false;
        }

        var firstLineParts = line.Split(',');
        if (firstLineParts.Length != 3)
        {
            return false;
        }

        try
        {
            var order = SortOrderExtensions.ParseSortOrder(firstLineParts[0].Trim());
            int maxItems = int.Parse(firstLineParts[1].Trim());
            double maxWeight = double.Parse(firstLineParts[2].Trim(), CultureInfo.InvariantCulture);

            // SAFETY: Validate configuration values
            if (maxItems <= 0 || maxWeight <= 0.0)
            {
                return false;
            }

            config = config with
            {
                Order = order,
                MaxItemsPerPack = maxItems,
                MaxWeightPerPack = maxWeight
            };
        }
        catch (Exception)
        {
            return false;
        }

        // Parse items
        while ((line = input.ReadLine()) != null && !string.IsNullOrEmpty(line))
        {
            var itemParts = line.Split(',');
            if (itemParts.Length != 4)
            {
                continue;
            }

            try
            {
                int id = int.Parse(itemParts[0].Trim());
                int length = int.Parse(itemParts[1].Trim());
                int quantity = int.Parse(itemParts[2].Trim());
                double weight = double.Parse(itemParts[3].Trim(), CultureInfo.InvariantCulture);

                // SAFETY: Validate item values
                if (length <= 0 || quantity <= 0 || weight < 0.0)
                {
                    continue; // Skip invalid items
                }

                items.Add(new Item(id, length, quantity, weight));
            }
            catch (Exception)
            {
                // Skip invalid lines
                continue;
            }
        }

        return true;
    }
}
