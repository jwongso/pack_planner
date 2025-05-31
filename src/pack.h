#ifndef PACK_H
#define PACK_H

#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include "item.h"

namespace pplanner {

class pack {
private:
    int m_pack_id;
    std::vector<item> m_items;
    int m_max_items;
    double m_max_weight;

    // Cached values for performance
    mutable int m_cached_item_count = -1;
    mutable double m_cached_weight = -1.0;
    mutable bool m_cache_valid = false;

    static constexpr double WEIGHT_EPSILON = 1e-9;

public:
    pack(int id, int max_items, double max_weight)
        : m_pack_id(id), m_max_items(max_items), m_max_weight(max_weight) {
        m_items.reserve(max_items); // Optimize memory allocation
    }

    // Add item to pack, returns remaining quantity if item couldn't fit completely
    // int add_item(const item& item_obj) {
    //     int remaining_quantity = item_obj.get_quantity();

    //     while (remaining_quantity > 0 && can_add_more_items()) {
    //         int quantity_to_add = calculate_max_addable_quantity(item_obj, remaining_quantity);

    //         if (quantity_to_add <= 0) break;

    //         m_items.emplace_back(item_obj.with_quantity(quantity_to_add));
    //         remaining_quantity -= quantity_to_add;
    //         invalidate_cache();
    //     }

    //     return remaining_quantity;
    // }

    int add_item(const item& item_obj) {
        int quantity_to_add = calculate_max_addable_quantity(item_obj, item_obj.get_quantity());
        if (quantity_to_add <= 0) return item_obj.get_quantity();

        m_items.emplace_back(item_obj.with_quantity(quantity_to_add));
        // Incremental cache update instead of full invalidation
        m_cached_item_count += quantity_to_add;
        m_cached_weight += item_obj.get_weight() * quantity_to_add;

        return item_obj.get_quantity() - quantity_to_add;
    }

    // Check if pack can accommodate more items
    bool can_add_more_items() const {
        return get_current_item_count() < m_max_items &&
               (get_current_weight() + WEIGHT_EPSILON < m_max_weight);
    }

    // Optimized getters with caching
    int get_current_item_count() const {
        if (!m_cache_valid) {
            update_cache();
        }
        return m_cached_item_count;
    }

    double get_max_weight() const {
        return m_max_weight;
    }

    double get_current_weight() const {
        if (!m_cache_valid) {
            update_cache();
        }
        return m_cached_weight;
    }

    int get_pack_length() const {
        int max_length = 0;
        for (const auto& item_obj : m_items) {
            max_length = std::max(max_length, item_obj.get_length());
        }
        return max_length;
    }

    int get_max_items() const { return m_max_items; }

    const std::vector<item>& get_items() const { return m_items; }

    // Output pack information with better formatting
    void print_pack() const {
        std::cout << "Pack Number: " << m_pack_id << std::endl;
        for (const auto& item_obj : m_items) {
            std::cout << item_obj.get_id() << "," << item_obj.get_length()
            << "," << item_obj.get_quantity() << "," << std::fixed
            << std::setprecision(3) << item_obj.get_weight() << std::endl;
        }
        std::cout << "Pack Length: " << get_pack_length()
                  << ", Pack Weight: " << std::fixed << std::setprecision(2)
                  << get_current_weight() << std::endl;
    }

    bool is_empty() const {
        return m_items.empty();
    }

    int get_remaining_item_slots() const {
        return m_max_items - get_current_item_count();
    }

    // Additional utility methods
    double get_weight_utilization() const {
        return m_max_weight > 0 ? (get_current_weight() / m_max_weight) * 100.0 : 0.0;
    }

    double get_item_utilization() const {
        return m_max_items > 0 ? (double(get_current_item_count()) / m_max_items) * 100.0 : 0.0;
    }

    size_t get_item_type_count() const {
        return m_items.size();
    }

private:
    void update_cache() const {
        m_cached_item_count = 0;
        m_cached_weight = 0.0;

        for (const auto& item_obj : m_items) {
            m_cached_item_count += item_obj.get_quantity();
            m_cached_weight += item_obj.get_total_weight();
        }

        m_cache_valid = true;
    }

    void invalidate_cache() {
        m_cache_valid = false;
    }

    int calculate_max_addable_quantity(const item& item_obj, int requested_quantity) const {
        // Calculate maximum quantity based on weight constraint
        double remaining_weight = m_max_weight - get_current_weight();
        int max_by_weight = static_cast<int>(remaining_weight / item_obj.get_weight());

        // Calculate maximum quantity based on item count constraint
        int max_by_items = get_remaining_item_slots();

        // Return the minimum of all constraints
        return std::min({requested_quantity, max_by_weight, max_by_items});
    }
};

} // pplanner

#endif // PACK_H
