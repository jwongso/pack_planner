#pragma once

#include <vector>
#include <string>
#include "PackPlanner.h"
#include "Timer.h"

struct BenchmarkResult {
    int size;
    std::string sortOrder;
    double sortingTime;
    double packingTime;
    double totalTime;
    int itemsPerSecond;
    int totalPacks;
    double utilizationPercent;
};

class Benchmark {
public:
    Benchmark();
    
    // Run benchmark with different sizes and sort orders
    void runBenchmark();
    
    // Generate test data for benchmarking
    std::vector<Item> generateTestData(int size);
    
    // Run single benchmark test
    BenchmarkResult runSingleBenchmark(int size, SortOrder sortOrder);
    
    // Output benchmark results
    void outputBenchmarkResults(const std::vector<BenchmarkResult>& results);

private:
    PackPlanner planner_;
    Timer totalTimer_;
    
    // Default benchmark configuration
    static constexpr int MAX_ITEMS_PER_PACK = 40;
    static constexpr double MAX_WEIGHT_PER_PACK = 500.0;
    
    // Benchmark sizes
    static const std::vector<int> BENCHMARK_SIZES;
    static const std::vector<SortOrder> SORT_ORDERS;
};
