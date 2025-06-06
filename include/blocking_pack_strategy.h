#pragma once

#include "pack_strategy.h"

constexpr int SAFE_MAX_LENGTH = 1'000'000;
constexpr int SAFE_MAX_QUANTITY = 10'000;
constexpr double SAFE_MAX_WEIGHT = 1e6;

/**
 * @brief Blocking (synchronous) pack strategy
 * Sequential processing of items one by one
 */
class blocking_pack_strategy : public pack_strategy {
public:
    /**
     * @brief Pack items into packs sequentially
     * @param items Items to pack
     * @param max_items Maximum items per pack
     * @param max_weight Maximum weight per pack
     * @return std::vector<pack> Vector of packs
     */
    std::vector<pack> pack_items(const std::vector<item>& items,
                                 int max_items,
                                 double max_weight) override {
        // Reject invalid constraints
        if (max_items <= 0 || max_weight <= 0.0) {
            return {};
        }

        std::vector<pack> packs;
        // Pre-allocate based on empirical ratio to avoid reallocations
        packs.reserve(std::max<size_t>(64, static_cast<size_t>(items.size() * 0.00222) + 16));
        int pack_number = 1;
        packs.emplace_back(pack_number);

        for (const auto& item : items) {
            // Skip items with invalid properties
            if (item.get_length() < 0 || item.get_weight() < 0.0 || item.get_quantity() <= 0) {
                continue;
            }
            // Reject extreme edge-case values close to limits
            if (item.get_length() > SAFE_MAX_LENGTH ||
                item.get_quantity() > SAFE_MAX_QUANTITY ||
                item.get_weight() > SAFE_MAX_WEIGHT) {
                // log warning, reject item
                continue;
            }

            int remaining_quantity = item.get_quantity();
            if (item.get_weight() > max_weight || 1 > max_items) {
                packs.emplace_back(++pack_number); // represent attempt to pack
                continue;
            }
            while (remaining_quantity > 0) {
                pack& current_pack = packs.back();
                int added_quantity = current_pack.add_partial_item(
                    item.get_id(), item.get_length(), remaining_quantity,
                    item.get_weight(), max_items, max_weight);
                if (added_quantity > 0) {
                    remaining_quantity -= added_quantity;
                } else {
                    packs.emplace_back(++pack_number);
                }
            }
        }
        return packs;
    }

    std::string get_name() const override {
        return "Blocking";
    }
};
