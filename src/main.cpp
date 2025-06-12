#include <iostream>
#include <fstream>
#include <string>
#include "pack_planner.h"
#include "benchmark.h"
#include <CLI/CLI.hpp>

void printUsage(const std::string& programName) {
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << programName << "                    - Read from standard input" << std::endl;
    std::cout << "  " << programName << " <input_file>       - Read from input file" << std::endl;
    std::cout << "  " << programName << " --benchmark        - Run performance benchmark" << std::endl;
}

/**
 * @brief Parse input from stream into configuration and items
 * @param input Input stream to read from
 * @param config Configuration to populate
 * @param items Vector to populate with items
 * @return bool True if parsing was successful
 */
[[nodiscard]] static bool parse_input(std::istream& input, pack_planner_config& config, std::vector<item>& items) {
    std::string line;

    // Parse first line: sort order, max items, max weight
    if (!std::getline(input, line) || line.empty()) {
        return false;
    }

    std::istringstream first_line(line);
    std::string sort_order_str, max_items_str, max_weight_str;

    if (!std::getline(first_line, sort_order_str, ',') ||
        !std::getline(first_line, max_items_str, ',') ||
        !std::getline(first_line, max_weight_str)) {
        return false;
    }

    config.order = parse_sort_order(sort_order_str);
    config.max_items_per_pack = std::stoi(max_items_str);
    config.max_weight_per_pack = std::stod(max_weight_str);

    // Parse items
    while (std::getline(input, line) && !line.empty()) {
        std::istringstream item_line(line);
        std::string id_str, length_str, quantity_str, weight_str;

        if (std::getline(item_line, id_str, ',') &&
            std::getline(item_line, length_str, ',') &&
            std::getline(item_line, quantity_str, ',') &&
            std::getline(item_line, weight_str)) {

            int id = std::stoi(id_str);
            int length = std::stoi(length_str);
            int quantity = std::stoi(quantity_str);
            double weight = std::stod(weight_str);

            items.emplace_back(id, length, quantity, weight);
        }
    }

    return true;
}

int main(int argc, char* argv[]) {
    CLI::App app{"Pack Planner - Efficiently pack items into containers"};

    // Input options
    std::string input_file;
    bool use_stdin = false;

    // Pack strategy options
    std::string strategy_str = "bff"; // blocking first fit
    int thread_count = 4;

    // Benchmark option
    bool run_benchmark = false;

    // Add CLI options
    app.add_flag("-i,--stdin", use_stdin, "Read input from standard input");
    app.add_option("-f,--file", input_file, "Input file path");
    app.add_option("-s,--strategy", strategy_str, "Packing strategy (blocking first fit or parallel first fit)")
        ->check(CLI::IsMember({"bff", "pff"}));
    app.add_option("-t,--threads", thread_count, "Number of threads for parallel strategy")
        ->check(CLI::Range(1, 64));
    app.add_flag("-b,--benchmark", run_benchmark, "Run performance benchmark");

    // Parse command line
    CLI11_PARSE(app, argc, argv);

    // Run benchmark if requested
    if (run_benchmark) {
        benchmark benchmark;
        benchmark.run_benchmark();
        return 0;
    }

    // Set up planner and configuration
    pack_planner planner;
    pack_planner_config config;
    std::vector<item> items;

    // Set strategy type from command line
    config.type = (strategy_str == "pff") ?
                  strategy_type::PARALLEL_FIRST_FIT :
                  strategy_type::BLOCKING_FIRST_FIT;

    // Set thread count for parallel strategy
    config.thread_count = thread_count;

    bool parse_success = false;

    // Handle input source
    if (use_stdin || (input_file.empty() && !run_benchmark)) {
        // Read from standard input
        parse_success = parse_input(std::cin, config, items);
    } else if (!input_file.empty()) {
        // Read from file
        std::ifstream inputFile(input_file);
        if (!inputFile.is_open()) {
            std::cerr << "Error: Could not open input file: " << input_file << std::endl;
            return 1;
        }
        parse_success = parse_input(inputFile, config, items);
        inputFile.close();
    }

    if (!parse_success) {
        std::cerr << "Error: Failed to parse input." << std::endl;
        return 1;
    }

    if (items.empty()) {
        std::cerr << "Error: No items to pack." << std::endl;
        return 1;
    }

    // Plan packs
    pack_planner_result result = planner.plan_packs(config, items);

    // Output results
    planner.output_results(result.packs);

    // Output strategy and timing information
    std::cout << "\nPacking Summary:" << std::endl;
    std::cout << "Strategy: " << result.strategy_name << std::endl;
    std::cout << "Sorting time: " << result.sorting_time << " ms" << std::endl;
    std::cout << "Packing time: " << result.packing_time << " ms" << std::endl;
    std::cout << "Total time: " << result.total_time << " ms" << std::endl;
    std::cout << "Utilization: " << result.utilization_percent << "%" << std::endl;

    return 0;
}
