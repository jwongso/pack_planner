#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <vector>
#include <random>
#include <iostream>
#include <iomanip>
#include "item.h"
#include "pack_planner.h"
#include "timer.h"

namespace utils {

// Benchmark data generator for testing
class benchmark_data_generator {
public:
    static std::vector<pplanner::item> generate_test_data(size_t count, int seed = 42) {
        std::mt19937 gen(seed);
        std::uniform_int_distribution<> id_dist(1000, 9999);
        std::uniform_int_distribution<> length_dist(100, 10000);
        std::uniform_int_distribution<> quantity_dist(1, 100);
        std::uniform_real_distribution<> weight_dist(0.1, 50.0);

        std::vector<pplanner::item> items;
        items.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            items.emplace_back(
                id_dist(gen),
                length_dist(gen),
                quantity_dist(gen),
                weight_dist(gen)
            );
        }

        return items;
    }
};

// Comprehensive benchmark runner
class benchmark_runner {
public:
    static void run_performance_test() {
        std::vector<size_t> test_sizes = {100, 1000, 5000, 10000, 50000};

        std::cout << "\n=== PERFORMANCE BENCHMARK ===" << std::endl;
        std::cout << "Running C++ Performance Benchmarks..." << std::endl;
        std::cout << "Size\tSorting(ms)\tPacking(ms)\tTotal(ms)\tItems/sec" << std::endl;
        std::cout << "----\t-----------\t-----------\t---------\t---------" << std::endl;

        for (size_t size : test_sizes) {
            auto items = benchmark_data_generator::generate_test_data(size);

            pplanner::pack_planner planner(50, 500.0, pplanner::sort_order::SHORT_TO_LONG, false); // Disable built-in timing for benchmark

            // Add items to planner
            for (const auto& item : items) {
                planner.add_item(item);
            }

            // Manual timing for benchmark
            utils::timer sort_timer, pack_timer, total_timer;

            total_timer.start();

            // Sort timing
            sort_timer.start();
            auto sorter = pplanner::sorter_factory::create_sorter(pplanner::sort_order::SHORT_TO_LONG);
            std::vector<pplanner::item> sorted_items = items; // Copy for sorting
            sorter->sort(sorted_items);
            sort_timer.stop();

            // Pack timing
            pack_timer.start();
            planner.plan_packs();
            pack_timer.stop();

            total_timer.stop();

            std::cout << size << "\t"
                      << std::fixed << std::setprecision(3)
                      << sort_timer.get_milliseconds() << "\t\t"
                      << pack_timer.get_milliseconds() << "\t\t"
                      << total_timer.get_milliseconds() << "\t\t"
                      << std::setprecision(0)
                      << (double)size / total_timer.get_microseconds() * 1000000
                      << std::endl;
        }
        std::cout << "=============================" << std::endl;
    }

    // Warmup runs to stabilize performance
    static void warmup() {
        std::cout << "Running warmup..." << std::endl;
        for (int i = 0; i < 5; ++i) {
            auto items = benchmark_data_generator::generate_test_data(1000);
            pplanner::pack_planner planner(50, 500.0, pplanner::sort_order::NATURAL, false);
            for (const auto& item : items) {
                planner.add_item(item);
            }
            planner.plan_packs();
        }
        std::cout << "Warmup complete." << std::endl;
    }
};

} // utils

#endif // BENCHMARK_H
