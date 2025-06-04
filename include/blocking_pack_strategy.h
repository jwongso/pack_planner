#pragma once

#include "pack_strategy.h"

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
        std::vector<pack> packs;
        // Pre-allocate based on empirical ratio to avoid reallocations
        packs.reserve(std::max<size_t>(64, static_cast<size_t>(items.size() * 0.00222) + 16));
        int pack_number = 1;
        packs.emplace_back(pack_number);

        for (const auto& item : items) {
            int remaining_quantity = item.get_quantity();

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
