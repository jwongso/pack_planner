#include <iostream>
#include <fstream>
#include <string>
#include "PackPlanner.h"
#include "Benchmark.h"

void printUsage(const std::string& programName) {
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << programName << "                    - Read from standard input" << std::endl;
    std::cout << "  " << programName << " <input_file>       - Read from input file" << std::endl;
    std::cout << "  " << programName << " --benchmark        - Run performance benchmark" << std::endl;
}

int main(int argc, char* argv[]) {
    // Check for benchmark mode
    if (argc == 2 && std::string(argv[1]) == "--benchmark") {
        Benchmark benchmark;
        benchmark.runBenchmark();
        return 0;
    }
    
    // Check for help
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        printUsage(argv[0]);
        return 0;
    }
    
    PackPlanner planner;
    PackPlannerConfig config;
    std::vector<Item> items;
    
    bool parseSuccess = false;
    
    if (argc == 1) {
        // Read from standard input
        parseSuccess = planner.parseInput(std::cin, config, items);
    } else if (argc == 2) {
        // Read from file
        std::ifstream inputFile(argv[1]);
        if (!inputFile.is_open()) {
            std::cerr << "Error: Could not open input file: " << argv[1] << std::endl;
            return 1;
        }
        parseSuccess = planner.parseInput(inputFile, config, items);
        inputFile.close();
    } else {
        std::cerr << "Error: Invalid number of arguments." << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    if (!parseSuccess) {
        std::cerr << "Error: Failed to parse input." << std::endl;
        return 1;
    }
    
    if (items.empty()) {
        std::cerr << "Error: No items to pack." << std::endl;
        return 1;
    }
    
    // Plan packs
    PackPlannerResult result = planner.planPacks(config, items);
    
    // Output results
    planner.outputResults(result.packs);
    
    return 0;
}
