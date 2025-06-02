#include "Benchmark.h"
#include <iostream>
#include <iomanip>
#include <random>

const std::vector<int> Benchmark::BENCHMARK_SIZES = {100000, 1000000, 5000000, 10000000, 20000000};
const std::vector<SortOrder> Benchmark::SORT_ORDERS = {SortOrder::NATURAL, SortOrder::LONG_TO_SHORT, SortOrder::SHORT_TO_LONG};

Benchmark::Benchmark() {
}

void Benchmark::runBenchmark() {
    std::cout << "=== PERFORMANCE BENCHMARK ===" << std::endl;
    std::cout << "Running C++ Performance Benchmarks..." << std::endl;
    
    std::vector<BenchmarkResult> allResults;
    
    totalTimer_.start();
    
    for (SortOrder sortOrder : SORT_ORDERS) {
        std::cout << "Size    Sorting(ms) Packing(ms) Total(ms)   Items/sec   Packs   Util%" << std::endl;
        std::cout << "----------------------------------------------------------------------" << std::endl;
        
        for (int size : BENCHMARK_SIZES) {
            BenchmarkResult result = runSingleBenchmark(size, sortOrder);
            allResults.push_back(result);
            
            std::cout << std::left << std::setw(8) << size
                      << std::left << std::setw(12) << result.sortOrder
                      << std::fixed << std::setprecision(3)
                      << std::left << std::setw(12) << result.packingTime
                      << std::left << std::setw(12) << result.totalTime
                      << std::left << std::setw(12) << result.itemsPerSecond
                      << std::left << std::setw(8) << result.totalPacks
                      << std::setprecision(1) << result.utilizationPercent << "%" << std::endl;
        }
        std::cout << std::endl;
    }
    
    double totalBenchmarkTime = totalTimer_.stop();
    
    std::cout << "Total benchmark execution: " << std::fixed << std::setprecision(3) 
              << totalBenchmarkTime << " ms (" << static_cast<long long>(totalBenchmarkTime * 1000) << " μs)" << std::endl;
}

std::vector<Item> Benchmark::generateTestData(int size) {
    std::vector<Item> items;
    items.reserve(size);
    
    std::random_device rd;
    std::mt19937 gen(42); // Fixed seed for reproducible results
    std::uniform_int_distribution<> lengthDist(1000, 10000);
    std::uniform_int_distribution<> quantityDist(1, 100);
    std::uniform_real_distribution<> weightDist(1.0, 20.0);
    
    for (int i = 0; i < size; ++i) {
        int id = 1000 + i;
        int length = lengthDist(gen);
        int quantity = quantityDist(gen);
        double weight = weightDist(gen);
        
        items.emplace_back(id, length, quantity, weight);
    }
    
    return items;
}

BenchmarkResult Benchmark::runSingleBenchmark(int size, SortOrder sortOrder) {
    BenchmarkResult result;
    result.size = size;
    result.sortOrder = sortOrderToString(sortOrder);
    
    // Generate test data
    std::vector<Item> items = generateTestData(size);
    
    // Configure pack planner
    PackPlannerConfig config;
    config.sortOrder = sortOrder;
    config.maxItemsPerPack = MAX_ITEMS_PER_PACK;
    config.maxWeightPerPack = MAX_WEIGHT_PER_PACK;
    
    // Run pack planning
    PackPlannerResult planResult = planner_.planPacks(config, items);
    
    // Fill benchmark result
    result.sortingTime = planResult.sortingTime;
    result.packingTime = planResult.packingTime;
    result.totalTime = planResult.totalTime;
    result.totalPacks = static_cast<int>(planResult.packs.size());
    result.utilizationPercent = planResult.utilizationPercent;
    
    // Calculate items per second
    if (result.totalTime > 0) {
        result.itemsPerSecond = static_cast<int>((planResult.totalItems * 1000.0) / result.totalTime);
    } else {
        result.itemsPerSecond = 0;
    }
    
    return result;
}

void Benchmark::outputBenchmarkResults(const std::vector<BenchmarkResult>& results) {
    std::cout << "Size    Sorting(ms) Packing(ms) Total(ms)   Items/sec   Packs   Util%" << std::endl;
    std::cout << "----------------------------------------------------------------------" << std::endl;
    
    for (const auto& result : results) {
        std::cout << std::left << std::setw(8) << result.size
                  << std::left << std::setw(12) << result.sortOrder
                  << std::fixed << std::setprecision(3)
                  << std::left << std::setw(12) << result.packingTime
                  << std::left << std::setw(12) << result.totalTime
                  << std::left << std::setw(12) << result.itemsPerSecond
                  << std::left << std::setw(8) << result.totalPacks
                  << std::setprecision(1) << result.utilizationPercent << "%" << std::endl;
    }
}
