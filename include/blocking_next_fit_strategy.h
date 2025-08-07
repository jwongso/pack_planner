#pragma once

#include "pack_strategy.h"

class next_fit_pack_strategy : public pack_strategy {
public:
    std::vector<pack> pack_items(const std::vector<item>& items,
                                 int max_items,
                                 double max_weight) override {
        // Validate constraints
        max_items = std::max(1, max_items);
        max_weight = std::max(0.1, max_weight);

        std::vector<pack> packs;
        const size_t max_safe_reserve = std::min<size_t>(100000, items.size() / 10 + 1000);
        packs.reserve(max_safe_reserve);
        int pack_number = 1;
        packs.emplace_back(pack_number);

        for (const auto& item : items) {
            if (item.get_quantity() <= 0) continue;

            int remaining_quantity = item.get_quantity();

            while (remaining_quantity > 0) {
                // Only check the current (last) pack
                pack& current_pack = packs.back();

                int added = current_pack.add_partial_item(
                    item.get_id(), item.get_length(), remaining_quantity,
                    item.get_weight(), max_items, max_weight);

                if (added > 0) {
                    remaining_quantity -= added;
                } else {
                    // Current pack is full, create new one
                    if (item.get_weight() > max_weight) {
                        // Item too heavy
                        break;
                    }

                    if (packs.size() >= max_safe_reserve) {
                        break;
                    }

                    packs.emplace_back(++pack_number);
                }
            }
        }

        return packs;
    }

    std::string get_name() const override {
        return "Next-Fit";
    }
};
