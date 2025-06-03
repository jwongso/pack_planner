#include <iostream>
#include <fstream>
#include <string>
#include "pack_planner.h"
#include "benchmark.h"

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
    //Check for benchmark mode
    if (argc == 2 && std::string(argv[1]) == "--benchmark") {
        benchmark benchmark;
        benchmark.run_benchmark();
        return 0;
    }
    
    // Check for help
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        printUsage(argv[0]);
        return 0;
    }
    
    pack_planner planner;
    pack_planner_config config;
    std::vector<item> items;
    
    bool parse_success = false;
    
    if (argc == 1) {
        // Read from standard input
        parse_success = parse_input(std::cin, config, items);
    } else if (argc == 2) {
        // Read from file
        std::ifstream inputFile(argv[1]);
        if (!inputFile.is_open()) {
            std::cerr << "Error: Could not open input file: " << argv[1] << std::endl;
            return 1;
        }
        parse_success = parse_input(inputFile, config, items);
        inputFile.close();
    } else {
        std::cerr << "Error: Invalid number of arguments." << std::endl;
        printUsage(argv[0]);
        return 1;
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
    
    return 0;
}
