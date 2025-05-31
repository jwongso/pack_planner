#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <map>
#include <vector>
#include <random>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include "item.h"
#include "pack_planner.h"
#include "parallel_pack_planner.h"
#include "timer.h"

namespace utils {

// Enhanced benchmark data generator with more realistic test scenarios
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

    // Generate stress test data with edge cases
    static std::vector<pplanner::item> generate_stress_test_data(size_t count, int seed = 42) {
        std::mt19937 gen(seed);
        std::vector<pplanner::item> items;
        items.reserve(count);

        // Mix of different item characteristics
        for (size_t i = 0; i < count; ++i) {
            int pattern = i % 4;
            switch (pattern) {
            case 0: // Heavy items
                items.emplace_back(1000 + i, 5000, 1, 45.0);
                break;
            case 1: // Light items, high quantity
                items.emplace_back(2000 + i, 1000, 50, 1.0);
                break;
            case 2: // Medium items
                items.emplace_back(3000 + i, 3000, 10, 15.0);
                break;
            case 3: // Random items
                std::uniform_int_distribution<> length_dist(100, 8000);
                std::uniform_int_distribution<> quantity_dist(1, 30);
                std::uniform_real_distribution<> weight_dist(2.0, 25.0);
                items.emplace_back(4000 + i, length_dist(gen), quantity_dist(gen), weight_dist(gen));
                break;
            }
        }

        return items;
    }

    // Generate problematic test cases for debugging
    static std::vector<pplanner::item> generate_problematic_cases() {
        return {
            pplanner::item(1001, 6200, 50, 9.653),   // Heavy, high quantity
            pplanner::item(2001, 7200, 90, 11.21),   // Very heavy, very high quantity
            pplanner::item(3001, 8200, 30, 19.653),  // Super heavy
            pplanner::item(4001, 5000, 100, 4.5),    // Light but high quantity
            pplanner::item(5001, 10000, 1, 49.9)     // Single very heavy item
        };
    }
};

// Comprehensive benchmark runner with multiple test scenarios
class benchmark_runner {
public:
    static void run_performance_test() {
        std::vector<size_t> test_sizes = {100, 1000, 5000, 10000, 15000};

        std::cout << "\n=== PERFORMANCE BENCHMARK ===" << std::endl;
        std::cout << "Running C++ Performance Benchmarks..." << std::endl;
        std::cout << std::left << std::setw(8) << "Size"
                  << std::setw(12) << "Sorting(ms)"
                  << std::setw(12) << "Packing(ms)"
                  << std::setw(12) << "Total(ms)"
                  << std::setw(12) << "Items/sec"
                  << std::setw(8) << "Packs"
                  << "Util%" << std::endl;
        std::cout << std::string(70, '-') << std::endl;

        for (size_t size : test_sizes) {
            run_single_benchmark(size);
        }

        std::cout << std::string(70, '=') << std::endl;

        // Run stress tests
        run_stress_tests();

        // Run problematic case tests
        run_problematic_cases_test();

        run_optimized_benchmark();
        // run_parallel_benchmark(10000, 8);
        // run_parallel_benchmark(15000, 8);
        // run_scalability_test();
    }

    static void run_single_benchmark(size_t size) {
        auto items = benchmark_data_generator::generate_test_data(size);

        // Test with different pack configurations
        std::vector<std::pair<int, double>> configs = {
            {50, 500.0},    // Standard config
            {20, 300.0},    // Restrictive config
            {100, 1000.0}   // Generous config
        };

        for (const auto& config : configs) {
            run_benchmark_with_config(items, config.first, config.second, size);
            if (configs.size() > 1) break; // Only show first config in summary
        }
    }

    static void run_benchmark_with_config(const std::vector<pplanner::item>& items,
                                          int max_items, double max_weight, size_t size) {
        pplanner::pack_planner planner(max_items, max_weight, pplanner::sort_order::SHORT_TO_LONG, false);
        //pplanner::parallel_pack_planner planner(max_items, max_weight, pplanner::sort_order::LONG_TO_SHORT, false, 99999, 20);

        // Add items to planner
        for (const auto& item : items) {
            planner.add_item(item);
        }

        // Measure timing
        utils::timer total_timer;
        total_timer.start();

        try {
            planner.plan_packs();
            total_timer.stop();

            // Calculate statistics
            double total_ms = total_timer.get_milliseconds();
            double items_per_sec = total_ms > 0 ? (double)size / total_ms * 1000.0 : 0;

            // Get pack statistics
            size_t pack_count = planner.get_pack_count();
            double avg_utilization = calculate_average_utilization(planner.get_packs(), max_weight);

            std::cout << std::left << std::setw(8) << size
                      << std::setw(12) << std::fixed << std::setprecision(3) << "N/A"
                      << std::setw(12) << total_ms
                      << std::setw(12) << total_ms
                      << std::setw(12) << std::setprecision(0) << items_per_sec
                      << std::setw(8) << pack_count
                      << std::setprecision(1) << avg_utilization << "%" << std::endl;

        } catch (const std::exception& e) {
            total_timer.stop();
            std::cout << std::left << std::setw(8) << size << "ERROR: " << e.what() << std::endl;
        }
    }

    static void run_parallel_benchmark(size_t size, unsigned thread_count) {
        auto items = benchmark_data_generator::generate_test_data(size);

        std::cout << "\nParallel Benchmark (Threads: " << thread_count << ")" << std::endl;
        std::cout << std::left << std::setw(8) << "Size"
                  << std::setw(12) << "Time(ms)"
                  << std::setw(12) << "Speedup"
                  << std::setw(12) << "Packs"
                  << "Util%" << std::endl;
        std::cout << std::string(50, '-') << std::endl;

        // Test with standard config
        run_parallel_benchmark_with_config(items, 50, 500.0, size, thread_count);
    }

    static void run_parallel_benchmark_with_config(const std::vector<pplanner::item>& items,
                                                   int max_items, double max_weight,
                                                   size_t size, unsigned thread_count) {
        utils::timer timer;
        timer.start();

        try {
            pplanner::parallel_pack_planner planner(max_items, max_weight,
                                                    pplanner::sort_order::SHORT_TO_LONG,
                                                    false, // disable timing
                                                    99999, // max packs
                                                    thread_count);

            // Add items
            for (const auto& item : items) {
                planner.add_item(item);
            }

            // Run parallel packing
            planner.parallel_pack_items();
            timer.stop();

            // Calculate statistics
            double parallel_time = timer.get_milliseconds();
            double speedup = 0.0;

            // Compare with sequential if we have baseline
            static std::map<size_t, double> sequential_times;
            if (sequential_times.count(size)) {
                speedup = sequential_times[size] / parallel_time;
            }

            // Get pack statistics
            size_t pack_count = planner.get_pack_count();
            double avg_utilization = calculate_average_utilization(planner.get_packs(), max_weight);

            std::cout << std::left << std::setw(8) << size
                      << std::setw(12) << std::fixed << std::setprecision(3) << parallel_time
                      << std::setw(12) << std::setprecision(2) << speedup
                      << std::setw(12) << pack_count
                      << std::setprecision(1) << avg_utilization << "%" << std::endl;

        } catch (const std::exception& e) {
            timer.stop();
            std::cout << std::left << std::setw(8) << size << "ERROR: " << e.what() << std::endl;
        }
    }

    static void run_scalability_test() {
        std::vector<size_t> test_sizes = {1000, 10000, 15000, 20000};
        std::vector<unsigned> thread_counts = {4, 8, 12, 16};

        std::cout << "\n=== PARALLEL SCALABILITY TEST ===" << std::endl;

        for (size_t size : test_sizes) {
            // First get sequential time
            auto items = benchmark_data_generator::generate_test_data(size);
            run_benchmark_with_config(items, 50, 500.0, size);

            // Store sequential time for comparison
            utils::timer seq_timer;
            seq_timer.start();
            pplanner::pack_planner planner(50, 500.0, pplanner::sort_order::SHORT_TO_LONG, false);
            for (const auto& item : items) planner.add_item(item);
            planner.plan_packs();
            seq_timer.stop();
            double seq_time = seq_timer.get_milliseconds();

            std::cout << "\nSize: " << size << " (Seq: " << seq_time << "ms)" << std::endl;
            std::cout << "Threads | Time(ms) | Speedup | Efficiency" << std::endl;
            std::cout << "--------+----------+---------+-----------" << std::endl;

            for (unsigned threads : thread_counts) {
                utils::timer par_timer;
                par_timer.start();
                pplanner::parallel_pack_planner planner(50, 500.0,
                                                        pplanner::sort_order::SHORT_TO_LONG,
                                                        false, 99999, threads);
                for (const auto& item : items) planner.add_item(item);
                planner.parallel_pack_items();
                par_timer.stop();

                double par_time = par_timer.get_milliseconds();
                double speedup = seq_time / par_time;
                double efficiency = (speedup / threads) * 100.0;

                std::cout << std::setw(7) << threads << " | "
                          << std::setw(8) << std::fixed << std::setprecision(2) << par_time << " | "
                          << std::setw(7) << std::setprecision(2) << speedup << " | "
                          << std::setw(6) << std::setprecision(1) << efficiency << "%" << std::endl;
            }
        }
    }

    static double measure_sequential(const std::vector<pplanner::item>& items) {
        utils::timer timer;
        timer.start();

        pplanner::pack_planner planner(50, 500.0, pplanner::sort_order::SHORT_TO_LONG, false);
        for (const auto& item : items) planner.add_item(item);
        planner.plan_packs();

        timer.stop();
        return timer.get_milliseconds();
    }

    static double measure_parallel(pplanner::parallel_pack_planner& planner, const std::vector<pplanner::item>& items) {
        utils::timer timer;
        timer.start();

        for (const auto& item : items) planner.add_item(item);
        planner.parallel_pack_items();

        timer.stop();
        return timer.get_milliseconds();
    }

    static void run_optimized_benchmark() {
        std::vector<size_t> sizes = {1000, 5000, 10000, 15000, 20000};
        std::vector<unsigned> threads = {1, 4, 8, 12, 16};

        std::cout << "Optimized Parallel Packing Benchmark\n";
        std::cout << "Size\tThreads\tTime(ms)\tSpeedup\tEfficiency\tPacks\n";

        for (size_t size : sizes) {
            auto items = benchmark_data_generator::generate_test_data(size);
            double seq_time = measure_sequential(items);

            for (unsigned t : threads) {
                pplanner::parallel_pack_planner planner(50, 500.0, pplanner::sort_order::SHORT_TO_LONG, false, 99999, t);
                double par_time = measure_parallel(planner, items);

                double speedup = seq_time / par_time;
                double efficiency = (speedup / t) * 100.0;

                std::cout << size << "\t" << t << "\t"
                          << std::fixed << std::setprecision(2) << par_time << "\t"
                          << std::setprecision(2) << speedup << "\t"
                          << std::setprecision(1) << efficiency << "%\t"
                          << planner.get_pack_count() << "\n";
            }
        }
    }

    static void run_stress_tests() {
        std::cout << "\n=== STRESS TESTS ===" << std::endl;

        auto stress_items = benchmark_data_generator::generate_stress_test_data(1000);

        // Test with very restrictive constraints
        test_configuration("Restrictive", stress_items, 5, 50.0);
        test_configuration("Moderate", stress_items, 20, 200.0);
        test_configuration("Generous", stress_items, 100, 1000.0);
    }

    static void run_problematic_cases_test() {
        std::cout << "\n=== PROBLEMATIC CASES TEST ===" << std::endl;

        auto problem_items = benchmark_data_generator::generate_problematic_cases();

        // These should test edge cases that might cause infinite loops
        test_configuration("Heavy Items", problem_items, 20, 300.0);
        test_configuration("Tight Constraints", problem_items, 10, 150.0);
        test_configuration("Very Tight", problem_items, 5, 100.0);
    }

    static void test_configuration(const std::string& test_name,
                                   const std::vector<pplanner::item>& items,
                                   int max_items, double max_weight) {
        std::cout << "Testing " << test_name << " (max_items=" << max_items
                  << ", max_weight=" << max_weight << ")... ";

        try {
            pplanner::pack_planner planner(max_items, max_weight, pplanner::sort_order::NATURAL, false);

            for (const auto& item : items) {
                planner.add_item(item);
            }

            utils::timer test_timer;
            test_timer.start();
            planner.plan_packs();
            test_timer.stop();

            std::cout << "OK (" << test_timer.get_milliseconds() << "ms, "
                      << planner.get_pack_count() << " packs)" << std::endl;

        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << std::endl;
        }
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

private:
    static double calculate_average_utilization(const std::vector<pplanner::pack>& packs, double max_weight) {
        if (packs.empty()) return 0.0;

        double total_utilization = 0.0;
        size_t non_empty_count = 0;

        for (const auto& pack : packs) {
            if (!pack.is_empty()) {
                total_utilization += (pack.get_current_weight() / max_weight) * 100.0;
                non_empty_count++;
            }
        }

        return non_empty_count > 0 ? total_utilization / non_empty_count : 0.0;
    }
};

} // utils

#endif // BENCHMARK_H
