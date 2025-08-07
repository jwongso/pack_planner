#include "benchmark.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <map>

const std::vector<int> benchmark::BENCHMARK_SIZES = {100000, 1000000, 5000000, 10000000, 20000000};
const std::vector<sort_order> benchmark::SORT_ORDERS = {sort_order::NATURAL,
                                                        sort_order::LONG_TO_SHORT,
                                                        sort_order::SHORT_TO_LONG
};
const std::vector<strategy_type> benchmark::PACKING_STRATEGIES = {  strategy_type::BLOCKING_FIRST_FIT,
                                                                    strategy_type::PARALLEL_FIRST_FIT,
                                                                    strategy_type::LOCKFREE_FIRST_FIT,
                                                                    strategy_type::BLOCKING_NEXT_FIT
};
const std::vector<unsigned int> benchmark::THREAD_COUNTS = {0}; // 0 means use hardware concurrency

benchmark::benchmark() {
}

void benchmark::run_benchmarks() {
    run_benchmark_internal(THREAD_COUNTS);
}

void benchmark::run_benchmark_with_threads(const std::vector<unsigned int>& thread_counts) {
    run_benchmark_internal(thread_counts);
}

void benchmark::run_benchmark_internal(const std::vector<unsigned int>& thread_counts) {
    std::cout << "=== PERFORMANCE BENCHMARK ===" << std::endl;
    std::cout << "Running C++ Performance Benchmarks..." << std::endl;

    std::vector<benchmark_result> all_results;

    m_total_timer.start();

    for (strategy_type strategy : PACKING_STRATEGIES) {
        // For blocking strategy, only use thread count 1
        std::vector<unsigned int> strategy_thread_counts;
        if (strategy == strategy_type::BLOCKING_FIRST_FIT  ||
            strategy == strategy_type::BLOCKING_NEXT_FIT) {
            strategy_thread_counts.push_back(1);
        } else {
            strategy_thread_counts = thread_counts;
        }

        for (unsigned int threads : strategy_thread_counts) {
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

void benchmark::benchmark_sorts() {
    std::cout << "\n=== SORTING ALGORITHM BENCHMARKS ===\n";
    std::cout << "Comparing different sorting algorithms for pack planning\n\n";

    // Run single-threaded benchmarks
    benchmark_single_threaded_sorts();

    // Run multi-threaded benchmarks
    benchmark_multi_threaded_sorts();

    std::cout << "\nSorting benchmark complete.\n";
}

void benchmark::benchmark_single_threaded_sorts() {
    std::cout << "\n--- SINGLE-THREADED SORTING BENCHMARKS ---\n";
    std::cout << "Testing serial sorting algorithms\n\n";

    // Ensure single-threaded execution
    optimized_sort::set_thread_count(1);

    // Store results for summary
    struct SortResult {
        std::string algorithm;
        size_t dataset_size;
        double duration_ms;
        double throughput;
    };
    std::vector<SortResult> results;

    for (size_t size : BENCHMARK_SIZES) {
        std::cout << "\nBenchmarking " << size << " items:\n";
        std::cout << std::string(70, '-') << "\n";

        // Generate random items with fixed seed for consistency
        std::vector<item> original_items;
        original_items.reserve(size);

        std::mt19937 gen(42); // Fixed seed for reproducibility
        std::uniform_int_distribution<> length_dist(100, 10000);
        std::uniform_int_distribution<> quantity_dist(1, 10);
        std::uniform_real_distribution<> weight_dist(0.1, 50.0);

        for (size_t i = 0; i < size; ++i) {
            original_items.emplace_back(i, length_dist(gen), quantity_dist(gen), weight_dist(gen));
        }

        // Lambda to benchmark a sorting algorithm
        auto benchmark_sort = [&](const std::string& name, auto sort_func) {
            auto items = original_items;
            auto start = std::chrono::high_resolution_clock::now();
            sort_func(items);
            auto end = std::chrono::high_resolution_clock::now();

            auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            double duration_ms = duration_us / 1000.0;

            std::cout << "  " << std::left << std::setw(20) << name
                      << std::right << std::setw(10) << std::fixed << std::setprecision(2)
                      << duration_ms << " ms";

            if (size > 0 && duration_us > 0) {
                double items_per_sec = (size * 1000000.0) / duration_us;
                std::cout << " (" << format_throughput(items_per_sec) << ")";
                results.push_back({name, size, duration_ms, items_per_sec});
            }
            std::cout << std::endl;
        };

        // Test single-threaded algorithms
        benchmark_sort("std::sort", [](std::vector<item>& items) {
            std::sort(items.begin(), items.end());
        });

        benchmark_sort("RadixSort", [](std::vector<item>& items) {
            optimized_sort::RadixSort::sort_by_length(items, true);
        });

        benchmark_sort("SIMDRadixSort", [](std::vector<item>& items) {
            optimized_sort::SIMDRadixSort::sort_by_length(items, true);
        });

        benchmark_sort("SIMDRadixSortV2", [](std::vector<item>& items) {
            optimized_sort::SIMDRadixSortV2::sort_by_length(items, true);
        });

        benchmark_sort("RadixQuickSort", [](std::vector<item>& items) {
            optimized_sort::RadixQuickSort::sort_by_length(items, true);
        });

        benchmark_sort("IntroRadixSort", [](std::vector<item>& items) {
            optimized_sort::IntroRadixSort::sort_by_length(items, true);
        });

        benchmark_sort("CountingSort", [](std::vector<item>& items) {
            optimized_sort::CountingSort::sort_by_length(items, true);
        });

        benchmark_sort("std::stable_sort", [](std::vector<item>& items) {
            std::stable_sort(items.begin(), items.end());
        });
    }

    // Print summary with top 3 algorithms
    std::cout << "\n--- SINGLE-THREADED SUMMARY ---\n";
    std::cout << "Top 3 algorithms for each dataset size:\n\n";
    std::cout << std::left << std::setw(15) << "Dataset Size"
              << std::setw(22) << "Algorithm"
              << std::setw(18) << "Throughput"
              << "Improvement vs std::sort" << "\n";
    std::cout << std::string(75, '-') << "\n";

    for (size_t size : BENCHMARK_SIZES) {
        // Find std::sort performance for comparison
        auto std_sort_it = std::find_if(results.begin(), results.end(),
                                        [size](const SortResult& r) {
                                            return r.dataset_size == size && r.algorithm == "std::sort";
                                        });

        double std_sort_throughput = std_sort_it != results.end() ? std_sort_it->throughput : 1.0;

        // Get all results for this size and sort by throughput
        std::vector<SortResult> size_results;
        std::copy_if(results.begin(), results.end(), std::back_inserter(size_results),
                     [size](const SortResult& r) { return r.dataset_size == size; });

        std::sort(size_results.begin(), size_results.end(),
                  [](const SortResult& a, const SortResult& b) {
                      return a.throughput > b.throughput;
                  });

        // Print top 3
        for (size_t i = 0; i < std::min(size_t(3), size_results.size()); ++i) {
            const auto& result = size_results[i];
            double improvement = result.throughput / std_sort_throughput;

            std::cout << std::left << std::setw(15) << size
                      << std::setw(22) << result.algorithm
                      << std::setw(18) << format_throughput(result.throughput)
                      << std::fixed << std::setprecision(2) << improvement << "x\n";
        }
        std::cout << "\n";
    }
}

void benchmark::benchmark_multi_threaded_sorts() {
    std::cout << "\n\n--- MULTI-THREADED SORTING BENCHMARKS ---\n";
    std::cout << "Testing parallel sorting algorithms with varying thread counts\n\n";

    std::vector<unsigned int> thread_counts = {2, 4, 8, 16, 24};

    // Store results for analysis
    struct ParallelResult {
        std::string algorithm;
        unsigned int threads;
        size_t dataset_size;
        double duration_ms;
        double throughput;
        double speedup; // vs single-threaded
    };
    std::vector<ParallelResult> results;

    // First, get baseline single-threaded performance for comparison
    optimized_sort::set_thread_count(1);
    std::map<std::pair<size_t, std::string>, double> baseline_throughput;

    for (size_t size : BENCHMARK_SIZES) {
        if (size < 100000) continue; // Skip small datasets

        std::vector<item> items;
        items.reserve(size);
        std::mt19937 gen(42);
        std::uniform_int_distribution<> length_dist(100, 10000);
        std::uniform_int_distribution<> quantity_dist(1, 10);
        std::uniform_real_distribution<> weight_dist(0.1, 50.0);

        for (size_t i = 0; i < size; ++i) {
            items.emplace_back(i, length_dist(gen), quantity_dist(gen), weight_dist(gen));
        }

        // Get baseline for each algorithm type
        auto get_baseline = [&](const std::string& name, auto sort_func) {
            auto test_items = items;
            auto start = std::chrono::high_resolution_clock::now();
            sort_func(test_items);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            baseline_throughput[{size, name}] = (size * 1000000.0) / duration_us;
        };

        get_baseline("RadixSort", [](std::vector<item>& items) {
            optimized_sort::RadixSort::sort_by_length(items, true);
        });

        get_baseline("CountingSort", [](std::vector<item>& items) {
            optimized_sort::CountingSort::sort_by_length(items, true);
        });

        get_baseline("MergeSort", [](std::vector<item>& items) {
            std::sort(items.begin(), items.end());
        });
    }

    for (unsigned int num_threads : thread_counts) {
        std::cout << "\n=== Testing with " << num_threads << " threads ===\n";

        // Set thread count for parallel algorithms
        optimized_sort::set_thread_count(num_threads);

        for (size_t size : BENCHMARK_SIZES) {
            // Only test larger datasets for multi-threading (overhead not worth it for small sets)
            if (size < 100000) continue;

            std::cout << "\nDataset size: " << size << " items\n";

            // Generate test data
            std::vector<item> original_items;
            original_items.reserve(size);
            std::mt19937 gen(42);
            std::uniform_int_distribution<> length_dist(100, 10000);
            std::uniform_int_distribution<> quantity_dist(1, 10);
            std::uniform_real_distribution<> weight_dist(0.1, 50.0);

            for (size_t i = 0; i < size; ++i) {
                original_items.emplace_back(i, length_dist(gen), quantity_dist(gen), weight_dist(gen));
            }

            // Lambda to benchmark parallel algorithms
            auto benchmark_parallel = [&](const std::string& name, const std::string& baseline_name,
                                          auto sort_func) {
                auto items = original_items;

                auto start = std::chrono::high_resolution_clock::now();
                sort_func(items);
                auto end = std::chrono::high_resolution_clock::now();

                auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                double duration_ms = duration_us / 1000.0;
                double throughput = (size * 1000000.0) / duration_us;
                double speedup = throughput / baseline_throughput[{size, baseline_name}];

                std::cout << "  " << std::left << std::setw(20) << name
                          << "Time: " << std::right << std::setw(8) << std::fixed << std::setprecision(2)
                          << duration_ms << " ms"
                          << "  Throughput: " << std::setw(10) << format_throughput(throughput)
                          << "  Speedup: " << std::setprecision(2) << speedup << "x"
                          << std::endl;

                results.push_back({name, num_threads, size, duration_ms, throughput, speedup});
            };

            // Test parallel algorithms
            benchmark_parallel("ParallelRadixSort", "RadixSort", [](std::vector<item>& items) {
                optimized_sort::ParallelRadixSort::sort_by_length(items, true);
            });

            benchmark_parallel("LockFreeRadixSort", "RadixSort", [](std::vector<item>& items) {
                optimized_sort::LockFreeParallelRadixSort::sort_by_length(items, true);
            });

            benchmark_parallel("ParallelCountingSort", "CountingSort", [](std::vector<item>& items) {
                optimized_sort::ParallelCountingSort::sort_by_length(items, true);
            });

            benchmark_parallel("LockFreeCountingSort", "CountingSort", [](std::vector<item>& items) {
                optimized_sort::LockFreeParallelCountingSort::sort_by_length(items, true);
            });

            benchmark_parallel("ParallelMergeSort", "MergeSort", [](std::vector<item>& items) {
                optimized_sort::ParallelMergeSort::sort_by_length(items, true);
            });

            benchmark_parallel("ParallelSTL", "MergeSort", [](std::vector<item>& items) {
                optimized_sort::ParallelSTLSort::sort_by_length(items, true);
            });
        }
    }

    // Print scalability analysis
    std::cout << "\n--- MULTI-THREADED SCALABILITY ANALYSIS ---\n";
    std::cout << "Speedup vs single-threaded version (20M items):\n\n";

    std::cout << std::left << std::setw(22) << "Algorithm" << " | ";
    for (auto tc : thread_counts) {
        std::cout << std::right << std::setw(10) << (std::to_string(tc) + " threads") << " | ";
    }
    std::cout << "\n" << std::string(90, '-') << "\n";

    std::vector<std::string> parallel_algorithms = {"ParallelRadixSort", "ParallelCountingSort",
                                                    "ParallelMergeSort", "ParallelSTL"};

    for (const auto& algo : parallel_algorithms) {
        std::cout << std::left << std::setw(22) << algo << " | ";
        for (auto tc : thread_counts) {
            auto it = std::find_if(results.begin(), results.end(),
                                   [&](const ParallelResult& r) {
                                       return r.algorithm == algo && r.threads == tc && r.dataset_size == 20000000;
                                   });

            if (it != results.end()) {
                std::cout << std::right << std::setw(10) << std::fixed << std::setprecision(2)
                << it->speedup << "x | ";
            } else {
                std::cout << std::right << std::setw(10) << "N/A | ";
            }
        }
        std::cout << "\n";
    }

    // Find optimal thread count for each algorithm
    std::cout << "\n--- OPTIMAL THREAD COUNT ---\n";
    std::cout << "Best thread count for each algorithm (20M items):\n\n";

    for (const auto& algo : parallel_algorithms) {
        auto best_it = std::max_element(results.begin(), results.end(),
                                        [&](const ParallelResult& a, const ParallelResult& b) {
                                            if (a.algorithm != algo || a.dataset_size != 20000000) return true;
                                            if (b.algorithm != algo || b.dataset_size != 20000000) return false;
                                            return a.throughput < b.throughput;
                                        });

        if (best_it != results.end() && best_it->algorithm == algo && best_it->dataset_size == 20000000) {
            std::cout << std::left << std::setw(22) << algo
                      << "Best: " << best_it->threads << " threads"
                      << " (" << format_throughput(best_it->throughput) << ", "
                      << std::fixed << std::setprecision(2) << best_it->speedup << "x speedup)"
                      << std::endl;
        }
    }

    // Efficiency analysis
    std::cout << "\n--- PARALLEL EFFICIENCY ---\n";
    std::cout << "Efficiency = Speedup / Thread Count (20M items):\n\n";

    std::cout << std::left << std::setw(22) << "Algorithm" << " | ";
    for (auto tc : thread_counts) {
        std::cout << std::right << std::setw(10) << (std::to_string(tc) + " threads") << " | ";
    }
    std::cout << "\n" << std::string(90, '-') << "\n";

    for (const auto& algo : parallel_algorithms) {
        std::cout << std::left << std::setw(22) << algo << " | ";
        for (auto tc : thread_counts) {
            auto it = std::find_if(results.begin(), results.end(),
                                   [&](const ParallelResult& r) {
                                       return r.algorithm == algo && r.threads == tc && r.dataset_size == 20000000;
                                   });

            if (it != results.end()) {
                double efficiency = it->speedup / tc * 100.0;
                std::cout << std::right << std::setw(9) << std::fixed << std::setprecision(1)
                          << efficiency << "% | ";
            } else {
                std::cout << std::right << std::setw(10) << "N/A | ";
            }
        }
        std::cout << "\n";
    }
}

std::string benchmark::format_throughput(double items_per_second) {
    std::stringstream ss;

    if (items_per_second >= 1e9) {
        ss << std::fixed << std::setprecision(2) << (items_per_second / 1e9) << "B items/sec";
    } else if (items_per_second >= 1e6) {
        ss << std::fixed << std::setprecision(2) << (items_per_second / 1e6) << "M items/sec";
    } else if (items_per_second >= 1e3) {
        ss << std::fixed << std::setprecision(2) << (items_per_second / 1e3) << "K items/sec";
    } else {
        ss << std::fixed << std::setprecision(0) << items_per_second << " items/sec";
    }

    return ss.str();
}
