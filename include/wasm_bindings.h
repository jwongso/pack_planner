#pragma once
#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include "item.h"
#include "pack.h"
#include "blocking_pack_strategy.h"
#include <vector>
#include <string>

class PackPlanner {
public:
    std::vector<std::string> packItems(emscripten::val jsItems, int maxItems, double maxWeight) {
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

        blocking_pack_strategy strategy;
        auto packs = strategy.pack_items(items, maxItems, maxWeight);

        std::vector<std::string> results;
        for (const auto& p : packs) {
            results.push_back(p.to_string());
        }

        return results;
    }
};

EMSCRIPTEN_BINDINGS(pack_planner_module) {
    emscripten::class_<PackPlanner>("PackPlanner")
        .constructor<>()
        .function("packItems", &PackPlanner::packItems);

    emscripten::register_vector<std::string>("VectorString");
}
#endif
