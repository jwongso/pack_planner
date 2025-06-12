#include "benchmark.h"
#include <iostream>
#include <iomanip>
#include <random>

const std::vector<int> benchmark::BENCHMARK_SIZES = {100000, 1000000, 5000000, 10000000, 20000000};
const std::vector<sort_order> benchmark::SORT_ORDERS = {sort_order::NATURAL,
                                                        sort_order::LONG_TO_SHORT,
                                                        sort_order::SHORT_TO_LONG};
const std::vector<strategy_type> benchmark::PACKING_STRATEGIES = { strategy_type::BLOCKING_FIRST_FIT,
                                                                  strategy_type::PARALLEL_FIRST_FIT };
const std::vector<unsigned int> benchmark::THREAD_COUNTS = {0}; // 0 means use hardware concurrency

benchmark::benchmark() {
}

void benchmark::run_benchmark() {
    std::cout << "=== PERFORMANCE BENCHMARK ===" << std::endl;
    std::cout << "Running C++ Performance Benchmarks..." << std::endl;

    std::vector<benchmark_result> all_results;

    m_total_timer.start();

    for (strategy_type strategy : PACKING_STRATEGIES) {
        for (unsigned int threads : THREAD_COUNTS) {
            // Skip thread variations for blocking strategy
            if (strategy == strategy_type::BLOCKING_FIRST_FIT && threads > 0) {
                continue;
            }

            for (sort_order order : SORT_ORDERS) {
                std::cout << "Strategy: " <<
                    pack_strategy_factory::strategy_type_to_string(strategy);
                if (strategy == strategy_type::PARALLEL_FIRST_FIT) {
                    std::cout <<
                        " (Threads: " << (threads == 0 ? "Auto" : std::to_string(threads)) << ")";
                }
                std::cout << ", Order: " << sort_order_to_string(order) << std::endl;

                std::cout << "Size      Sort(ms)    Pack(ms)    Total(ms)   Items/sec   Packs       Util%" << std::endl;
                std::cout << "----------------------------------------------------------------------------" << std::endl;

                for (int size : BENCHMARK_SIZES) {
                    benchmark_result result = run_single_benchmark(size, order, strategy, threads);
                    all_results.push_back(result);

                    std::cout << std::left << std::setw(10) << size
                              << std::fixed << std::setprecision(3)
                              << std::left << std::setw(12) << result.sorting_time
                              << std::left << std::setw(12) << result.packing_time
                              << std::left << std::setw(12) << result.total_time
                              << std::left << std::setw(12) << result.items_per_second
                              << std::left << std::setw(12) << result.total_packs
                              << std::setprecision(1) << result.utilization_percent << "%" << std::endl;
                }
                std::cout << std::endl;
            }
        }
    }

    double total_benchmark_time = m_total_timer.stop();

    std::cout << "Total benchmark execution: " << std::fixed << std::setprecision(3)
              << total_benchmark_time << " ms (" <<
        static_cast<long long>(total_benchmark_time * 1000) << " μs)" << std::endl;
}

std::vector<item> benchmark::generate_test_data(int size) {
    std::vector<item> items;
    items.reserve(size);
    
    std::random_device rd;
    std::mt19937 gen(48); // Fixed seed for reproducible results
    std::uniform_int_distribution<> length_dist(500, 10000);
    std::uniform_int_distribution<> quantity_dist(10, 100);
    std::uniform_real_distribution<> lightweight_dist(0.5, 6.0);
    std::uniform_real_distribution<> heavyweight_dist(6.1, 30.0);
    
    for (int i = 0; i < size; ++i) {
        int id = 1000 + i;
        int length = length_dist(gen);
        int quantity = quantity_dist(gen);
        double weight = 0.0;
        // 70% lightweight items (sorting matters)
        // 30% heavyweight items (always split)
        if (i % 10 < 7) {
            weight = lightweight_dist(gen);  // 0.5-3.0kg
        } else {
            weight = heavyweight_dist(gen);  // 15-25kg
        }
        
        items.emplace_back(id, length, quantity, weight);
    }
    
    return items;
}

benchmark_result benchmark::run_single_benchmark(int size, sort_order order,
                                                 strategy_type strategy,
                                                 unsigned int num_threads) {
    benchmark_result result;
    result.size = size;
    result.order = sort_order_to_string(order);
    result.strategy = pack_strategy_factory::strategy_type_to_string(strategy);
    result.num_threads = num_threads;

    // Generate test data
    std::vector<item> items = generate_test_data(size);

    // Configure pack planner
    pack_planner_config config;
    config.order = order;
    config.max_items_per_pack = MAX_ITEMS_PER_PACK;
    config.max_weight_per_pack = MAX_WEIGHT_PER_PACK;
    config.type = strategy;
    config.thread_count = num_threads;

    // Run pack planning
    pack_planner_result plan_result = m_planner.plan_packs(config, items);

    // Fill benchmark result
    result.sorting_time = plan_result.sorting_time;
    result.packing_time = plan_result.packing_time;
    result.total_time = plan_result.total_time;
    result.total_packs = static_cast<int>(plan_result.packs.size());
    result.utilization_percent = plan_result.utilization_percent;

    // Calculate items per second
    if (result.total_time > 0) {
        result.items_per_second =
            static_cast<long long>((plan_result.total_items * 1000.0) / result.total_time);
    } else {
        result.items_per_second = 0;
    }

    return result;
}

void benchmark::output_benchmark_results(const std::vector<benchmark_result>& results) {
    std::cout << "Size    Strategy  Threads  Order    Sort(ms)  Pack(ms)  Total(ms) Items/sec Packs   Util%" << std::endl;
    std::cout << "----------------------------------------------------------------------------------------" << std::endl;

    for (const auto& result : results) {
        std::cout << std::left << std::setw(8) << result.size
                  << std::left << std::setw(10) << result.strategy
                  << std::left << std::setw(9) << (result.strategy == "Parallel" ?
                                                       (result.num_threads == 0 ? "Auto" :
                                                        std::to_string(result.num_threads)) : "-")
                  << std::left << std::setw(9) << result.order
                  << std::fixed << std::setprecision(3)
                  << std::left << std::setw(10) << result.sorting_time
                  << std::left << std::setw(10) << result.packing_time
                  << std::left << std::setw(10) << result.total_time
                  << std::left << std::setw(10) << result.items_per_second
                  << std::left << std::setw(8) << result.total_packs
                  << std::setprecision(1) << result.utilization_percent << "%" << std::endl;
    }
}
