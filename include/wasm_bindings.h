#pragma once
#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/html5.h>
#include "item.h"
#include "pack.h"
#include "pack_planner.h"
#include "benchmark.h"
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <random>
#include <algorithm>

// Profiler results structure
struct ProfilerResults {
    double cpu_score;
    double memory_bandwidth_mbps;
    double network_latency_ms;
    double network_bandwidth_mbps;
    double battery_level;
    bool is_charging;
    std::string recommendation;
    double confidence_score;
};

class SystemProfiler {
private:
    std::mt19937 rng;

    // CPU benchmarking using mathematical operations
    double benchmarkCPU() {
        auto start = std::chrono::high_resolution_clock::now();

        // Mathematical operations benchmark
        double result = 0.0;
        const int iterations = 1000000;

        for (int i = 0; i < iterations; ++i) {
            double x = static_cast<double>(i) / 1000.0;
            result += std::sin(x) * std::cos(x) + std::sqrt(x + 1.0) + std::log(x + 1.0);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        // Calculate operations per second and normalize to a score (higher is better)
        double ops_per_second = (iterations * 4.0) / (duration.count() / 1000000.0); // 4 ops per iteration
        return ops_per_second / 1000000.0; // Normalize to millions of ops per second
    }

    // Memory bandwidth testing
    double benchmarkMemoryBandwidth() {
        const size_t buffer_size = 64 * 1024 * 1024; // 64MB
        const int iterations = 10;

        std::vector<uint8_t> src(buffer_size);
        std::vector<uint8_t> dst(buffer_size);

        // Fill source buffer with random data
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        for (size_t i = 0; i < buffer_size; ++i) {
            src[i] = dist(rng);
        }

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            std::copy(src.begin(), src.end(), dst.begin());
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        // Calculate bandwidth in MB/s
        double total_bytes = buffer_size * iterations;
        double seconds = duration.count() / 1000000.0;
        return (total_bytes / (1024 * 1024)) / seconds;
    }

    // Network latency test (using fetch to a known endpoint)
    emscripten::val testNetworkLatency() {
        return emscripten::val::global("Promise").call<emscripten::val>("resolve", emscripten::val(50.0)); // Placeholder
    }

    // Battery status assessment
    emscripten::val getBatteryInfo() {
        // This will be handled in JavaScript and passed back
        return emscripten::val::object();
    }

public:
    SystemProfiler() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

    // Main profiling method
    emscripten::val profileSystem() {
        ProfilerResults results;

        // CPU benchmarking
        results.cpu_score = benchmarkCPU();

        // Memory bandwidth testing
        results.memory_bandwidth_mbps = benchmarkMemoryBandwidth();

        // These will be filled by JavaScript callbacks
        results.network_latency_ms = 0.0;
        results.network_bandwidth_mbps = 0.0;
        results.battery_level = 1.0;
        results.is_charging = true;

        // Make recommendation based on CPU and memory performance
        makeRecommendation(results);

        // Convert to JavaScript object
        emscripten::val jsResults = emscripten::val::object();
        jsResults.set("cpuScore", results.cpu_score);
        jsResults.set("memoryBandwidthMbps", results.memory_bandwidth_mbps);
        jsResults.set("networkLatencyMs", results.network_latency_ms);
        jsResults.set("networkBandwidthMbps", results.network_bandwidth_mbps);
        jsResults.set("batteryLevel", results.battery_level);
        jsResults.set("isCharging", results.is_charging);
        jsResults.set("recommendation", results.recommendation);
        jsResults.set("confidenceScore", results.confidence_score);

        return jsResults;
    }

    // Stress test for more intensive profiling
    emscripten::val stressTest(int duration_seconds) {
        auto start = std::chrono::high_resolution_clock::now();
        auto end_time = start + std::chrono::seconds(duration_seconds);

        double cpu_total = 0.0;
        double memory_total = 0.0;
        int iterations = 0;

        while (std::chrono::high_resolution_clock::now() < end_time) {
            cpu_total += benchmarkCPU();
            memory_total += benchmarkMemoryBandwidth();
            iterations++;
        }

        emscripten::val results = emscripten::val::object();
        results.set("avgCpuScore", cpu_total / iterations);
        results.set("avgMemoryBandwidth", memory_total / iterations);
        results.set("iterations", iterations);
        results.set("duration", duration_seconds);

        return results;
    }

private:
    void makeRecommendation(ProfilerResults& results) {
        // Simple scoring algorithm
        double cpu_weight = 0.4;
        double memory_weight = 0.3;
        double network_weight = 0.2;
        double battery_weight = 0.1;

        // Normalize scores (these thresholds would be calibrated based on real data)
        double cpu_normalized = std::min(results.cpu_score / 10.0, 1.0); // Assume 10 MOPS is excellent
        double memory_normalized = std::min(results.memory_bandwidth_mbps / 10000.0, 1.0); // 10GB/s is excellent
        double network_normalized = 1.0; // Will be updated when network tests are implemented
        double battery_normalized = results.battery_level;

        double overall_score = cpu_normalized * cpu_weight +
                              memory_normalized * memory_weight +
                              network_normalized * network_weight +
                              battery_normalized * battery_weight;

        results.confidence_score = overall_score;

        if (overall_score > 0.7) {
            results.recommendation = "CLIENT_PREFERRED";
        } else if (overall_score > 0.4) {
            results.recommendation = "HYBRID";
        } else {
            results.recommendation = "SERVER_PREFERRED";
        }
    }
};

class PackPlanner {
public:
    std::vector<std::string> packItems(emscripten::val jsItems, int maxItems, double maxWeight,
                                      int sortOrder = 0, int strategyType = 0, int threadCount = 4) {
        std::vector<item> items;

        // Convert JS array to C++ vector<item>
        unsigned length = jsItems["length"].as<unsigned>();
        for (unsigned i = 0; i < length; ++i) {
            emscripten::val jsItem = jsItems[i];
            items.emplace_back(
                jsItem[0].as<int>(),    // id
                jsItem[1].as<int>(),    // length
                jsItem[2].as<int>(),    // quantity
                jsItem[3].as<double>()  // weight
            );
        }

        // Create configuration
        pack_planner_config config;
        config.max_items_per_pack = maxItems;
        config.max_weight_per_pack = maxWeight;
        config.order = static_cast<sort_order>(sortOrder);
        config.type = static_cast<strategy_type>(strategyType);
        config.thread_count = threadCount;

        // Use pack_planner instead of directly using blocking_pack_strategy
        pack_planner planner;
        auto result = planner.plan_packs(config, items);

        std::vector<std::string> results;
        for (const auto& p : result.packs) {
            if (!p.is_empty()) {
                results.push_back(p.to_string());
            }
        }

        return results;
    }

    // Add a method to get the planning stats
    emscripten::val getPlanningStats(emscripten::val jsItems, int maxItems, double maxWeight,
                                    int sortOrder = 0, int strategyType = 0, int threadCount = 4) {
        std::vector<item> items;

        // Convert JS array to C++ vector<item>
        unsigned length = jsItems["length"].as<unsigned>();
        for (unsigned i = 0; i < length; ++i) {
            emscripten::val jsItem = jsItems[i];
            items.emplace_back(
                jsItem[0].as<int>(),    // id
                jsItem[1].as<int>(),    // length
                jsItem[2].as<int>(),    // quantity
                jsItem[3].as<double>()  // weight
            );
        }

        // Create configuration
        pack_planner_config config;
        config.max_items_per_pack = maxItems;
        config.max_weight_per_pack = maxWeight;
        config.order = static_cast<sort_order>(sortOrder);
        config.type = static_cast<strategy_type>(strategyType);
        config.thread_count = threadCount;

        // Use pack_planner
        pack_planner planner;
        auto result = planner.plan_packs(config, items);

        // Return stats as a JavaScript object
        emscripten::val stats = emscripten::val::object();
        stats.set("sortingTime", result.sorting_time);
        stats.set("packingTime", result.packing_time);
        stats.set("totalTime", result.total_time);
        stats.set("totalItems", result.total_items);
        stats.set("utilizationPercent", result.utilization_percent);
        stats.set("strategyName", result.strategy_name);
        stats.set("packCount", result.packs.size());

        return stats;
    }

    // Add benchmark method for WASM
    emscripten::val runBenchmark(int size, int sortOrder, int strategyType, int threadCount) {
        benchmark bench;
        benchmark_result result = bench.run_single_benchmark(
            size,
            static_cast<sort_order>(sortOrder),
            static_cast<strategy_type>(strategyType),
            threadCount
        );

        // Return results as a JavaScript object
        emscripten::val jsResult = emscripten::val::object();
        jsResult.set("size", result.size);
        jsResult.set("order", result.order);
        jsResult.set("strategy", result.strategy);
        jsResult.set("numThreads", result.num_threads);
        jsResult.set("sortingTime", result.sorting_time);
        jsResult.set("packingTime", result.packing_time);
        jsResult.set("totalTime", result.total_time);
        jsResult.set("itemsPerSecond", result.items_per_second);
        jsResult.set("totalPacks", result.total_packs);
        jsResult.set("utilizationPercent", result.utilization_percent);

        return jsResult;
    }
};

EMSCRIPTEN_BINDINGS(pack_planner_module) {
    emscripten::class_<PackPlanner>("PackPlanner")
        .constructor<>()
        .function("packItems", &PackPlanner::packItems)
        .function("getPlanningStats", &PackPlanner::getPlanningStats)
        .function("runBenchmark", &PackPlanner::runBenchmark);

    emscripten::class_<SystemProfiler>("SystemProfiler")
        .constructor<>()
        .function("profileSystem", &SystemProfiler::profileSystem)
        .function("stressTest", &SystemProfiler::stressTest);

    emscripten::register_vector<std::string>("VectorString");

    // Register enum values for JavaScript
    emscripten::enum_<sort_order>("SortOrder")
        .value("NATURAL", sort_order::NATURAL)
        .value("SHORT_TO_LONG", sort_order::SHORT_TO_LONG)
        .value("LONG_TO_SHORT", sort_order::LONG_TO_SHORT);

    emscripten::enum_<strategy_type>("StrategyType")
        .value("BLOCKING", strategy_type::BLOCKING_FIRST_FIT)
        .value("PARALLEL", strategy_type::PARALLEL_FIRST_FIT);
}
#endif
