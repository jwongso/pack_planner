#ifndef PACK_PLANNER_H
#define PACK_PLANNER_H

#include <vector>
#include <iostream>
#include <limits>
#include <cmath>
#include "item.h"
#include "pack.h"
#include "sorter.h"
#include "timer.h"

namespace pplanner {

// Main pack planner class with integrated timing and optimizations
class pack_planner {
protected:
    int m_max_items;
    double m_max_weight;
    sort_order m_sort_order;
    std::vector<item> m_items;
    std::vector<pack> m_packs;
    bool m_enable_timing;
    int m_max_packs;

    // Floating-point comparison epsilon
    static constexpr double WEIGHT_EPSILON = 1e-9;

public:
    // Fixed constructor signature to match usage
    pack_planner(int max_items, double max_weight, sort_order order, bool enable_timing = true, int max_packs = 999999)
        : m_max_items(max_items), m_max_weight(max_weight),
        m_sort_order(order), m_enable_timing(enable_timing), m_max_packs(max_packs) {

        // Reserve space for better performance
        m_items.reserve(1000);
        m_packs.reserve(100);
    }

    void add_item(const item& item_obj) {
        m_items.push_back(item_obj);
    }

    // void plan_packs() {
    //     if (m_items.empty()) return;

    //     // Pre-validate all items against pack constraints
    //     validate_items();

    //     utils::timer sort_timer, pack_timer;

    //     if (m_enable_timing) {
    //         std::cout << "\n=== TIMING INFORMATION ===" << std::endl;
    //     }

    //     // Sort items according to the specified order
    //     if (m_enable_timing) sort_timer.start();
    //     auto sorter = sorter_factory::create_sorter(m_sort_order);
    //     sorter->sort(m_items);
    //     if (m_enable_timing) {
    //         sort_timer.stop();
    //         sort_timer.print_time("Sorting");
    //     }

    //     // Pack items with optimized algorithm
    //     if (m_enable_timing) pack_timer.start();
    //     pack_items_optimized();
    //     if (m_enable_timing) {
    //         pack_timer.stop();
    //         pack_timer.print_time("Packing");

    //         print_timing_summary(sort_timer, pack_timer);
    //     }
    // }

    void plan_packs() {
        if (m_items.empty()) return;

        // Pre-validate all items against pack constraints
        validate_items();

        utils::timer sort_timer, pack_timer;

        if (m_enable_timing) {
            std::cout << "\n=== TIMING INFORMATION ===" << std::endl;
        }

        // Phase 1: Primary sorting according to specified order
        if (m_enable_timing) sort_timer.start();

        auto sorter = sorter_factory::create_sorter(m_sort_order);
        sorter->sort(m_items);

        // Phase 2: Secondary sorting by weight (heavier items first)
        // This helps pack heavier items first for better weight distribution
        std::stable_sort(m_items.begin(), m_items.end(),
                         [](const item& a, const item& b) {
                             return a.get_weight() > b.get_weight();
                         });

        // Phase 3: Tertiary sorting by quantity (larger quantities first)
        // This helps reduce fragmentation when splitting items
        if (m_items.size() < 100000) {
            std::stable_sort(m_items.begin(), m_items.end(),
                             [](const item& a, const item& b) {
                                 return a.get_quantity() > b.get_quantity();
                             });
        }

        if (m_enable_timing) {
            sort_timer.stop();
            sort_timer.print_time("Sorting (multi-phase)");
        }

        // Pack items with optimized algorithm
        if (m_enable_timing) pack_timer.start();
        pack_items_optimized();
        if (m_enable_timing) {
            pack_timer.stop();
            pack_timer.print_time("Packing");

            print_timing_summary(sort_timer, pack_timer);
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

    // Getters for testing/debugging
    size_t get_pack_count() const { return m_packs.size(); }
    const std::vector<pack>& get_packs() const { return m_packs; }

protected:
    void validate_items() const {
        for (const auto& item_obj : m_items) {
            if (item_obj.get_weight() > m_max_weight + WEIGHT_EPSILON) {
                throw std::runtime_error("Item " + std::to_string(item_obj.get_id()) +
                                         " (weight: " + std::to_string(item_obj.get_weight()) +
                                         ") exceeds maximum pack weight (" +
                                         std::to_string(m_max_weight) + ")");
            }
        }
    }

    // void pack_items_optimized() {
    //     m_packs.clear();
    //     int pack_id = 1;

    //     for (const auto& item_obj : m_items) {
    //         int remaining_quantity = item_obj.get_quantity();

    //         while (remaining_quantity > 0) {
    //             // Find the most suitable pack using optimized algorithm
    //             pack* suitable_pack = find_first_fit_pack(item_obj);

    //             if (!suitable_pack) {
    //                 // Create new pack if needed
    //                 if (pack_id > m_max_packs) {
    //                     throw std::runtime_error("Maximum number of packs (" +
    //                                              std::to_string(m_max_packs) + ") exceeded");
    //                 }
    //                 m_packs.emplace_back(pack_id++, m_max_items, m_max_weight);
    //                 suitable_pack = &m_packs.back();
    //             }

    //             int previous_remaining = remaining_quantity;
    //             remaining_quantity = suitable_pack->add_item(item_obj.with_quantity(remaining_quantity));

    //             // Safety check to prevent infinite loops
    //             if (remaining_quantity == previous_remaining) {
    //                 throw std::runtime_error("Packing algorithm failed for item " +
    //                                          std::to_string(item_obj.get_id()) +
    //                                          ". Item cannot fit in any pack configuration.");
    //             }
    //         }
    //     }
    // }

    void pack_items_optimized() {
        m_packs.clear();
        int pack_id = 1;

        // Use best-fit for first 1000 items (better utilization)
        // Then switch to first-fit (faster for large datasets)
        const size_t best_fit_threshold = 1000;

        for (const auto& item_obj : m_items) {
            int remaining_quantity = item_obj.get_quantity();

            while (remaining_quantity > 0) {
                pack* suitable_pack = (m_items.size() < best_fit_threshold)
                ? find_best_fit_pack(item_obj)
                : find_first_fit_pack(item_obj);

                if (!suitable_pack) {
                    if (pack_id > m_max_packs) {
                        throw std::runtime_error("Maximum packs exceeded");
                    }
                    m_packs.emplace_back(pack_id++, m_max_items, m_max_weight);
                    suitable_pack = &m_packs.back();
                }

                // Optimize quantity calculation
                int max_possible = calculate_max_possible_quantity(*suitable_pack, item_obj, remaining_quantity);
                if (max_possible > 0) {
                    suitable_pack->add_item(item_obj.with_quantity(max_possible));
                    remaining_quantity -= max_possible;
                } else {
                    throw std::runtime_error("Packing error");
                }
            }
        }
    }

    // Optimized pack finding using best-fit decreasing strategy
    pack* find_best_fit_pack(const item& item_obj) {
        pack* best_pack = nullptr;
        double min_waste = std::numeric_limits<double>::max();

        for (auto& pack_obj : m_packs) {
            if (can_pack_fit_item_safe(pack_obj, item_obj)) {
                // Calculate waste (remaining capacity after adding this item)
                double remaining_weight = pack_obj.get_max_weight() - pack_obj.get_current_weight();
                double waste = remaining_weight - item_obj.get_weight();

                // Prefer packs with less waste (better utilization)
                if (waste < min_waste) {
                    min_waste = waste;
                    best_pack = &pack_obj;
                }
            }
        }

        return best_pack;
    }

    // Alternative: First-fit algorithm (faster but less optimal)
    pack* find_first_fit_pack(const item& item_obj) {
        for (auto& pack_obj : m_packs) {
            if (can_pack_fit_item_safe(pack_obj, item_obj)) {
                return &pack_obj;
            }
        }
        return nullptr;
    }

    int calculate_max_possible_quantity(const pack& p, const item& item_obj, int remaining_quantity) {
        double remaining_weight = p.get_max_weight() - p.get_current_weight();
        int max_by_weight = static_cast<int>(remaining_weight / item_obj.get_weight());
        int max_by_slots = p.get_remaining_item_slots();

        return std::min({remaining_quantity, max_by_weight, max_by_slots});
    }

    // Safe floating-point comparison for weight constraints
    bool can_pack_fit_item_safe(const pack& pack_obj, const item& item_obj) const {
        double remaining_weight = pack_obj.get_max_weight() - pack_obj.get_current_weight();

        // Use epsilon for floating-point comparison
        bool weight_fits = (remaining_weight + WEIGHT_EPSILON >= item_obj.get_weight());
        bool slots_available = (pack_obj.get_remaining_item_slots() > 0);

        return weight_fits && slots_available;
    }

    void print_timing_summary(const utils::timer& sort_timer, const utils::timer& pack_timer) const {
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

        // Additional statistics
        double total_utilization = calculate_total_utilization();
        std::cout << "Average pack utilization: " << std::fixed << std::setprecision(1)
                  << total_utilization << "%" << std::endl;
        std::cout << "=========================" << std::endl;
    }

    double calculate_total_utilization() const {
        if (m_packs.empty()) return 0.0;

        double total_utilization = 0.0;
        size_t non_empty_packs = 0;

        for (const auto& pack_obj : m_packs) {
            if (!pack_obj.is_empty()) {
                double weight_utilization = (pack_obj.get_current_weight() / pack_obj.get_max_weight()) * 100.0;
                total_utilization += weight_utilization;
                non_empty_packs++;
            }
        }

        return non_empty_packs > 0 ? total_utilization / non_empty_packs : 0.0;
    }
};

} // pplanner

#endif // PACK_PLANNER_H
