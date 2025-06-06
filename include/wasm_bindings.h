#pragma once
#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include "item.h"
#include "pack.h"
#include "pack_planner.h"
#include <vector>
#include <string>

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
};

EMSCRIPTEN_BINDINGS(pack_planner_module) {
    emscripten::class_<PackPlanner>("PackPlanner")
        .constructor<>()
        .function("packItems", &PackPlanner::packItems)
        .function("getPlanningStats", &PackPlanner::getPlanningStats);

    emscripten::register_vector<std::string>("VectorString");

    // Register enum values for JavaScript
    emscripten::enum_<sort_order>("SortOrder")
        .value("NATURAL", sort_order::NATURAL)
        .value("SHORT_TO_LONG", sort_order::SHORT_TO_LONG)
        .value("LONG_TO_SHORT", sort_order::LONG_TO_SHORT);

    emscripten::enum_<strategy_type>("StrategyType")
        .value("BLOCKING", strategy_type::BLOCKING)
        .value("PARALLEL", strategy_type::PARALLEL);
}
#endif
