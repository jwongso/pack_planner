#pragma once

#include <vector>
#include <string>
#include "pack_planner.h"
#include "timer.h"

struct benchmark_result {
    int size;
    std::string order;
    double sorting_time;
    double packing_time;
    double total_time;
    int items_per_second;
    int total_packs;
    double utilization_percent;
};

class benchmark {
public:
    benchmark();
    
    // Run benchmark with different sizes and sort orders
    void run_benchmark();
    
    // Generate test data for benchmarking
    std::vector<item> generate_test_data(int size);
    
    // Run single benchmark test
    benchmark_result run_single_benchmark(int size, sort_order sortOrder);
    
    // Output benchmark results
    void output_benchmark_results(const std::vector<benchmark_result>& results);

private:
    pack_planner m_planner;
    timer m_total_timer;
    
    // Default benchmark configuration
    static constexpr int MAX_ITEMS_PER_PACK = 40;
    static constexpr double MAX_WEIGHT_PER_PACK = 500.0;
    
    // benchmark sizes
    static const std::vector<int> BENCHMARK_SIZES;
    static const std::vector<sort_order> SORT_ORDERS;
};
