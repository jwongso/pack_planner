#pragma once

#include "pack_strategy.h"
#include <future>
#include <vector>
#include <algorithm>
#include <cmath>

/**
 * @brief Asynchronous pack strategy
 * Processes items asynchronously using std::async with deferred execution
 */
class async_pack_strategy : public pack_strategy {
private:
    /**
     * @brief Process a single item asynchronously
     * @param item Item to process
     * @param max_items Maximum items per pack
     * @param max_weight Maximum weight per pack
     * @param pack_number_start Starting pack number
     * @return std::vector<pack> Packs needed for this item
     */
    std::vector<pack> process_item_async(const item& item,
                                       int max_items,
                                       double max_weight,
                                       int pack_number_start) {
        std::vector<pack> item_packs;
        int remaining_quantity = item.get_quantity();
        int pack_number = pack_number_start;

        if (remaining_quantity > 0) {
            item_packs.emplace_back(pack_number);

            while (remaining_quantity > 0) {
                pack& current_pack = item_packs.back();
                int added_quantity = current_pack.add_partial_item(
                    item.get_id(), item.get_length(), remaining_quantity,
                    item.get_weight(), max_items, max_weight);

                if (added_quantity > 0) {
                    remaining_quantity -= added_quantity;
                } else {
                    item_packs.emplace_back(++pack_number);
                }
            }
        }

        return item_packs;
    }

public:
    std::vector<pack> pack_items(const std::vector<item>& items,
                               int max_items,
                               double max_weight) override {
        if (items.empty()) {
            return {};
        }

        // Create async tasks for each item
        std::vector<std::future<std::vector<pack>>> futures;
        futures.reserve(items.size());

        int pack_number_offset = 1;

        // Launch async tasks for each item
        for (const auto& item : items) {
            // Estimate how many packs this item might need
            int estimated_packs = std::max(1, static_cast<int>(
                std::ceil(static_cast<double>(item.get_quantity()) / max_items)));

            futures.emplace_back(std::async(std::launch::deferred,
                [this, item, max_items, max_weight, pack_number_offset]() {
                    return process_item_async(item, max_items, max_weight, pack_number_offset);
                }));

            pack_number_offset += estimated_packs;
        }

        // Collect results from futures
        std::vector<std::vector<pack>> all_item_packs;
        all_item_packs.reserve(futures.size());

        for (auto& future : futures) {
            all_item_packs.emplace_back(future.get());
        }

        // Now we need to merge the results intelligently
        // Since each item was processed independently, we need to consolidate packs
        return merge_async_results(all_item_packs, max_items, max_weight);
    }

    std::string get_name() const override {
        return "Async";
    }

private:
    /**
     * @brief Merge results from async processing
     * @param item_pack_groups Groups of packs from each item
     * @param max_items Maximum items per pack
     * @param max_weight Maximum weight per pack
     * @return std::vector<pack> Final consolidated packs
     */
    std::vector<pack> merge_async_results(
        const std::vector<std::vector<pack>>& item_pack_groups,
        int max_items,
        double max_weight) {

        std::vector<pack> final_packs;
        final_packs.reserve(100); // Initial estimate
        int pack_number = 1;
        final_packs.emplace_back(pack_number);

        // Process each item group sequentially for final consolidation
        for (const auto& item_packs : item_pack_groups) {
            for (const auto& source_pack : item_packs) {
                // Try to add each item from the source pack to our final packs
                for (const auto& item : source_pack.get_items()) {
                    int remaining_quantity = item.get_quantity();

                    while (remaining_quantity > 0) {
                        pack& current_pack = final_packs.back();
                        int added_quantity = current_pack.add_partial_item(
                            item.get_id(), item.get_length(), remaining_quantity,
                            item.get_weight(), max_items, max_weight);

                        if (added_quantity > 0) {
                            remaining_quantity -= added_quantity;
                        } else {
                            final_packs.emplace_back(++pack_number);
                        }
                    }
                }
            }
        }

        return final_packs;
    }
};
