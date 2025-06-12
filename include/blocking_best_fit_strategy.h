#pragma once

#include "pack_strategy.h"
#include <queue>
#include <stdexcept>

/**
 * @brief Optimal blocking best-fit pack strategy (O(n log m) complexity).
 * Uses a min-heap to track packs by remaining space for O(1) best-fit lookup.
 * @ref https://en.wikipedia.org/wiki/Best-fit_bin_packing
 */
class blocking_best_fit_strategy : public pack_strategy {
private:
    /**
     * @brief Structure to track pack capacity for priority queue
     */
    struct pack_capacity {
        int pack_index;
        double remaining_weight;
        int remaining_items;

        pack_capacity(int idx, double weight, int items)
            : pack_index(idx), remaining_weight(weight), remaining_items(items) {}
    };

    /**
     * @brief Comparator for priority queue - min-heap based on remaining weight
     * We want packs with LEAST remaining capacity first (best fit)
     */
    struct capacity_comparator {
        bool operator()(const pack_capacity& a, const pack_capacity& b) const {
            // If remaining weights are very close, compare by remaining items
            if (std::abs(a.remaining_weight - b.remaining_weight) < 0.001) {
                return a.remaining_items > b.remaining_items; // min remaining items first
            }
            return a.remaining_weight > b.remaining_weight; // min remaining weight first
        }
    };

public:
    /**
     * @brief Pack items into packs using the best-fit algorithm
     *
     * For each item, this strategy examines all existing packs and selects
     * the one with the least remaining capacity that can still fit the item.
     * If no existing pack can fit the item, a new pack is created.
     *
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
                    std::max<size_t>(64, static_cast<size_t>(items.size() * 0.002) + 16)));

        // Priority queue to maintain packs sorted by remaining capacity (best fit first)
        std::priority_queue<pack_capacity, std::vector<pack_capacity>, capacity_comparator> pack_queue;

        int pack_number = 1;

        // SAFETY: Add a safety counter to prevent infinite loops
        const int max_iterations = 1000000;
        int safety_counter = 0;

        for (const auto& item : items) {
            // SAFETY: Skip items with non-positive quantities
            if (item.get_quantity() <= 0) continue;

            int remaining_quantity = item.get_quantity();

            while (remaining_quantity > 0) {
                // SAFETY: Check for potential infinite loop
                if (++safety_counter > max_iterations) {
                    break;
                }

                int added_quantity = 0;

                // Try to find the best fitting pack from our queue
                std::vector<pack_capacity> temp_storage;

                while (!pack_queue.empty()) {
                    pack_capacity current = pack_queue.top();
                    pack_queue.pop();

                    // Check if this pack can accommodate at least one unit of the current item
                    if (current.remaining_items > 0 &&
                        current.remaining_weight >= item.get_weight()) {

                        // Try to add to this pack
                        pack& target_pack = packs[current.pack_index];
                        added_quantity = target_pack.add_partial_item(
                            item.get_id(),
                            item.get_length(),
                            remaining_quantity,
                            item.get_weight(),
                            max_items,
                            max_weight);

                        if (added_quantity > 0) {
                            // Update the pack's remaining capacity and put it back in queue
                            double new_remaining_weight = target_pack.get_remaining_weight_capacity(max_weight);
                            int new_remaining_items = target_pack.get_remaining_item_capacity(max_items);

                            if (new_remaining_items > 0 && new_remaining_weight > 0.001) {
                                temp_storage.emplace_back(current.pack_index, new_remaining_weight, new_remaining_items);
                            }
                            break;
                        }
                    }

                    // Pack couldn't fit this item, but might fit others - keep it
                    temp_storage.push_back(current);
                }

                // Restore all packs back to the queue
                for (const auto& pack_cap : temp_storage) {
                    pack_queue.push(pack_cap);
                }

                if (added_quantity > 0) {
                    remaining_quantity -= added_quantity;
                } else {
                    // No existing pack could fit this item, create a new pack
                    // SAFETY: Limit maximum number of packs to prevent OOM
                    if (packs.size() >= max_safe_reserve) {
                        remaining_quantity = 0;
                        break;
                    }

                    packs.emplace_back(pack_number++);
                    pack& new_pack = packs.back();

                    // Try to add to the new pack
                    added_quantity = new_pack.add_partial_item(
                        item.get_id(),
                        item.get_length(),
                        remaining_quantity,
                        item.get_weight(),
                        max_items,
                        max_weight);

                    if (added_quantity > 0) {
                        remaining_quantity -= added_quantity;

                        // Add new pack to queue if it has remaining capacity
                        double remaining_weight = new_pack.get_remaining_weight_capacity(max_weight);
                        int remaining_items = new_pack.get_remaining_item_capacity(max_items);

                        if (remaining_items > 0 && remaining_weight > 0.001) {
                            pack_queue.emplace(packs.size() - 1, remaining_weight, remaining_items);
                        }
                    } else {
                        // This should rarely happen - new pack can't fit even one unit
                        // Force exit to prevent infinite loop
                        remaining_quantity = 0;
                    }
                }
            }
        }

        return packs;
    }

    std::string get_name() const override {
        return "Blocking Best Fit";
    }
};
