#ifndef PACK_H
#define PACK_H

#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "item.h"

namespace pplanner {

class pack {
private:
    int m_pack_id;
    std::vector<item> m_items;
    int m_max_items;
    double m_max_weight;

public:
    pack(int id, int max_items, double max_weight)
        : m_pack_id(id), m_max_items(max_items), m_max_weight(max_weight) {}

    // Add item to pack, returns remaining quantity if item couldn't fit completely
    int add_item(const item& item_obj) {
        int remaining_quantity = item_obj.get_quantity();

        while (remaining_quantity > 0 && can_add_more_items()) {
            int quantity_to_add = std::min(remaining_quantity,
                std::min(get_remaining_item_slots(), get_max_quantity_by_weight(item_obj)));

            if (quantity_to_add <= 0) break;

            m_items.emplace_back(item_obj.with_quantity(quantity_to_add));
            remaining_quantity -= quantity_to_add;
        }

        return remaining_quantity;
    }

    // Check if pack can accommodate more items
    bool can_add_more_items() const {
        return get_current_item_count() < m_max_items && get_current_weight() < m_max_weight;
    }

    // Get current statistics
    int get_current_item_count() const {
        int count = 0;
        for (const auto& item_obj : m_items) {
            count += item_obj.get_quantity();
        }
        return count;
    }

    int get_max_weight() const {
        return m_max_weight;
    }

    double get_current_weight() const {
        double weight = 0.0;
        for (const auto& item_obj : m_items) {
            weight += item_obj.get_total_weight();
        }
        return weight;
    }

    int get_pack_length() const {
        int max_length = 0;
        for (const auto& item_obj : m_items) {
            max_length = std::max(max_length, item_obj.get_length());
        }
        return max_length;
    }

    // Output pack information
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

private:
    int get_max_quantity_by_weight(const item& item_obj) const {
        double remaining_weight = m_max_weight - get_current_weight();
        return static_cast<int>(remaining_weight / item_obj.get_weight());
    }
};

} // pplanner

#endif // PACK_H
