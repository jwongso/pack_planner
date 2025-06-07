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
        // SAFETY: Validate constraints to prevent infinite loops
        max_items = std::max(1, max_items);
        max_weight = std::max(0.1, max_weight);

        std::vector<pack> packs;
        // Pre-allocate based on empirical ratio to avoid reallocations
        // SAFETY: Limit initial allocation to prevent OOM with extreme values
        const size_t max_safe_reserve = 10000;
        packs.reserve(std::min(max_safe_reserve,
                    std::max<size_t>(64, static_cast<size_t>(items.size() * 0.00222) + 16)));
        int pack_number = 1;
        packs.emplace_back(pack_number);

        // SAFETY: Add a safety counter to prevent infinite loops
        const int max_iterations = 1000000; // Reasonable upper limit
        int safety_counter = 0;

        for (const auto& item : items) {
            // SAFETY: Skip items with non-positive quantities
            if (item.get_quantity() <= 0) continue;

            int remaining_quantity = item.get_quantity();

            while (remaining_quantity > 0) {
                // SAFETY: Check for potential infinite loop
                if (++safety_counter > max_iterations) {
                    // Force exit the loop if we've exceeded reasonable iterations
                    break;
                }

                pack& current_pack = packs.back();
                int added_quantity = current_pack.add_partial_item(
                    item.get_id(), item.get_length(), remaining_quantity,
                    item.get_weight(), max_items, max_weight);

                if (added_quantity > 0) {
                    remaining_quantity -= added_quantity;
                } else {
                    // SAFETY: Limit maximum number of packs to prevent OOM
                    if (packs.size() >= max_safe_reserve) {
                        // Force exit if we've created too many packs
                        remaining_quantity = 0;
                        break;
                    }
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
