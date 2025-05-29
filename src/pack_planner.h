#ifndef PACK_PLANNER_H
#define PACK_PLANNER_H

#include <vector>
#include <iostream>
#include "item.h"
#include "pack.h"
#include "sorter.h"
#include "timer.h"

namespace pplanner {

// Main pack planner class with integrated timing
class pack_planner {
protected:
    int m_max_items;
    double m_max_weight;
    sort_order m_sort_order;
    std::vector<item> m_items;
    std::vector<pack> m_packs;
    bool m_enable_timing;
    int m_max_packs;

public:
    pack_planner(int max_items, double max_weight, sort_order order, int max_packs = 1000)
        : m_max_items(max_items), m_max_weight(max_weight),
        m_sort_order(order), m_max_packs(max_packs) {}

    void add_item(const item& item_obj) {
        m_items.push_back(item_obj);
    }

    void plan_packs() {
        if (m_items.empty()) return;

        // Pre-validate all items against pack constraints
        for (const auto& item_obj : m_items) {
            if (item_obj.get_weight() > m_max_weight) {
                throw std::runtime_error("Item " + std::to_string(item_obj.get_id()) +
                                         " (weight: " + std::to_string(item_obj.get_weight()) +
                                         ") exceeds maximum pack weight (" +
                                         std::to_string(m_max_weight) + ")");
            }
        }

        utils::timer sort_timer, pack_timer;

        if (m_enable_timing) {
            std::cout << "\n=== TIMING INFORMATION ===" << std::endl;
        }

        // Sort items according to the specified order
        if (m_enable_timing) sort_timer.start();
        auto sorter = sorter_factory::create_sorter(m_sort_order);
        sorter->sort(m_items);
        if (m_enable_timing) {
            sort_timer.stop();
            sort_timer.print_time("Sorting");
        }

        // Pack items
        if (m_enable_timing) pack_timer.start();
        m_packs.clear();
        int pack_id = 1;

        for (const auto& item_obj : m_items) {
            int remaining_quantity = item_obj.get_quantity();

            while (remaining_quantity > 0) {
                // Find a pack that can accommodate at least one item
                pack* suitable_pack = find_suitable_pack_hybrid(item_obj);

                if (!suitable_pack) {
                    // Create new pack
                    m_packs.emplace_back(pack_id++, m_max_items, m_max_weight);
                    suitable_pack = &m_packs.back();
                }

                int previous_remaining = remaining_quantity;
                remaining_quantity = suitable_pack->add_item(item_obj.with_quantity(remaining_quantity));

                if (remaining_quantity == previous_remaining) {
                    throw std::runtime_error("Packing algorithm failed for item " +
                                             std::to_string(item_obj.get_id()) +
                                             ". This indicates a bug in the packing logic.");
                }
            }
        }
        if (m_enable_timing) {
            pack_timer.stop();
            pack_timer.print_time("Packing");

            long long total_time = sort_timer.get_microseconds() + pack_timer.get_microseconds();
            std::cout << "Total algorithm time: " << std::fixed << std::setprecision(3)
                      << total_time / 1000.0 << " ms (" << total_time << " μs)" << std::endl;
            std::cout << "Items processed: " << m_items.size() << std::endl;
            std::cout << "Packs created: " << m_packs.size() << std::endl;
            if (total_time > 0) {
                std::cout << "Performance: " << std::fixed << std::setprecision(0)
                          << (double)m_items.size() / total_time * 1000000
                          << " items/second" << std::endl;
            }
            std::cout << "=========================" << std::endl;
        }
    }

    void print_result() const {
        utils::timer output_timer;
        if (m_enable_timing) output_timer.start();

        for (const auto& pack_obj : m_packs) {
            if (!pack_obj.is_empty()) {
                pack_obj.print_pack();
            }
        }

        if (m_enable_timing) {
            output_timer.stop();
            std::cout << "\n";
            output_timer.print_time("Output generation");
        }
    }

    void set_timing_enabled(bool enabled) {
        m_enable_timing = enabled;
    }

protected:
    pack* find_suitable_pack() {
        for (auto& pack_obj : m_packs) {
            if (pack_obj.can_add_more_items()) {
                return &pack_obj;
            }
        }
        return nullptr;
    }

    pack* find_suitable_pack_first_fit(const item& item_obj) {
        for (auto& pack_obj : m_packs) {
            if (can_pack_fit_item(pack_obj, item_obj)) {
                return &pack_obj;
            }
        }
        return nullptr;
    }

    pack* find_suitable_pack_for(const item& item_obj) {
        pack* best_pack = nullptr;
        double max_used_weight = 0;

        for (auto& pack_obj : m_packs) {
            if (can_pack_fit_item(pack_obj, item_obj)) {
                double current_weight = pack_obj.get_current_weight();
                if (current_weight > max_used_weight) {
                    max_used_weight = current_weight;
                    best_pack = &pack_obj;
                }
            }
        }
        return best_pack;
    }

    pack* find_suitable_pack_hybrid(const item& item_obj) {
        pack* best_fit = nullptr;
        pack* first_fit = nullptr;
        double min_waste = std::numeric_limits<double>::max();

        for (auto& pack_obj : m_packs) {
            if (can_pack_fit_item(pack_obj, item_obj)) {
                if (!first_fit) first_fit = &pack_obj;  // Remember first suitable pack

                double waste = pack_obj.get_max_weight() - pack_obj.get_current_weight() - item_obj.get_weight();
                if (waste < min_waste) {
                    min_waste = waste;
                    best_fit = &pack_obj;
                }
            }
        }

        return best_fit ? best_fit : first_fit;
    }

    bool can_pack_fit_item(const pack& pack_obj, const item& item_obj) const {
        double remaining_weight = pack_obj.get_max_weight() - pack_obj.get_current_weight();

        // Use relative epsilon based on item weight magnitude
        double epsilon = std::max(1e-9, item_obj.get_weight() * 1e-12);

        return (remaining_weight + epsilon >= item_obj.get_weight()) &&
               (pack_obj.get_remaining_item_slots() > 0);
    }
};

}

#endif // PACK_PLANNER_H
