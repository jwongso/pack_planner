#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include "item.h"

/**
 * @brief Represents a pack containing multiple items
 */
class pack {
public:
    /**
     * @brief Construct a new pack object
     * @param pack_number The pack identifier
     */
    explicit pack(int pack_number) noexcept
        : m_pack_number(pack_number) {
        // Reserve some space to avoid initial reallocations
        m_items.reserve(8);
    }

    /**
     * @brief Add item to pack
     * @param item The item to add
     * @param max_items Maximum number of items allowed in the pack
     * @param max_weight Maximum weight allowed in the pack
     * @return bool True if successful, false if constraints violated
     */
    [[nodiscard]] bool add_item(const item& item, int max_items, double max_weight) noexcept {
        int new_quantity = m_total_items + item.get_quantity();
        double new_weight = m_total_weight + item.get_total_weight();

        if (new_quantity <= max_items && new_weight <= max_weight) {
            m_items.push_back(item);
            m_total_items = new_quantity;
            m_total_weight = new_weight;
            m_max_length = std::max(m_max_length, item.get_length());
            return true;
        }
        return false;
    }

    /**
     * @brief Try to add partial quantity of an item (optimized version)
     * @param item The item to add
     * @param remaining_quantity Remaining quantity of the item
     * @param max_items Maximum number of items allowed in the pack
     * @param max_weight Maximum weight allowed in the pack
     * @return int Number of items successfully added
     */
    [[nodiscard]] int add_partial_item(const item& item, int remaining_quantity, int max_items, double max_weight) noexcept {
        // Early exit if pack constraints are already exceeded
        if (m_total_items >= max_items || m_total_weight >= max_weight) {
            return 0;
        }

        const int max_by_items = max_items - m_total_items;

        // Only calculate weight constraint if we haven't already hit the item limit
        int can_add = std::min(max_by_items, remaining_quantity);
        if (can_add > 0) {
            // Calculate how many we can fit by weight
            const double remaining_weight = max_weight - m_total_weight;
            const int max_by_weight = static_cast<int>(remaining_weight / item.get_weight());
            can_add = std::min(can_add, max_by_weight);
        }

        if (can_add > 0) {
            m_items.emplace_back(item.get_id(), item.get_length(), can_add, item.get_weight());
            m_total_items += can_add;
            m_total_weight += can_add * item.get_weight();
            m_max_length = std::max(m_max_length, item.get_length());
        }

        return can_add;
    }

    /**
     * @brief Try to add partial quantity of an item (backward compatibility)
     * @param item The item to add
     * @param max_items Maximum number of items allowed in the pack
     * @param max_weight Maximum weight allowed in the pack
     * @return int Number of items successfully added
     */
    [[nodiscard]] int add_partial_item(const item& item, int max_items, double max_weight) noexcept {
        return add_partial_item(item, item.get_quantity(), max_items, max_weight);
    }

    /**
     * @brief Try to add partial quantity of an item using individual parameters
     * @param id The item ID
     * @param length The item length
     * @param quantity The item quantity
     * @param weight The item weight per piece
     * @param max_items Maximum number of items allowed in the pack
     * @param max_weight Maximum weight allowed in the pack
     * @return int Number of items successfully added
     */
    [[nodiscard]] int add_partial_item(int id, int length, int quantity, double weight,
                       int max_items, double max_weight) noexcept {
        const int max_by_items = max_items - m_total_items;
        const double weight_remaining = max_weight - m_total_weight;
        const int max_by_weight = static_cast<int>(weight_remaining / weight);
        const int can_add = std::min({max_by_items, max_by_weight, quantity});

        if (can_add > 0) {
            m_items.emplace_back(id, length, can_add, weight);
            m_total_items += can_add;
            m_total_weight += can_add * weight;
            m_max_length = std::max(m_max_length, length);
        }
        return can_add;
    }

    /**
     * @brief Check if the pack is full
     * @param max_items Maximum number of items allowed in the pack
     * @param max_weight Maximum weight allowed in the pack
     * @return bool True if the pack is full
     */
    [[nodiscard]] bool is_full(int max_items, double max_weight) const noexcept {
        // Floating-point precision fix by epsilon.
        return m_total_items >= max_items || m_total_weight >= max_weight - 1e-9;
    }

    // Getters
    /**
     * @brief Get the pack number
     * @return int The pack number
     */
    [[nodiscard]] int get_pack_number() const noexcept { return m_pack_number; }

    /**
     * @brief Get the items in the pack
     * @return const std::vector<Item>& Reference to the items vector
     */
    [[nodiscard]] const std::vector<item>& get_items() const noexcept { return m_items; }

    /**
     * @brief Get the total number of items in the pack
     * @return int Total number of items
     */
    [[nodiscard]] int get_total_items() const noexcept { return m_total_items; }

    /**
     * @brief Get the total weight of the pack
     * @return double Total weight
     */
    [[nodiscard]] double get_total_weight() const noexcept { return m_total_weight; }

    /**
     * @brief Get the maximum length of any item in the pack
     * @return int Maximum length
     */
    [[nodiscard]] int get_pack_length() const noexcept { return m_max_length; }

    /**
     * @brief Check if the pack is empty
     * @return bool True if the pack is empty
     */
    [[nodiscard]] bool is_empty() const noexcept { return m_items.empty(); }

    /**
     * @brief Get string representation of the pack
     * @return std::string The string representation
     */
    [[nodiscard]] std::string to_string() const {
        std::ostringstream oss;
        oss << "Pack Number: " << m_pack_number << "\n";

        for (const auto& i : m_items) {
            oss << i.to_string() << "\n";
        }

        oss << "Pack Length: " << get_pack_length()
            << ", Pack Weight: " << std::fixed << std::setprecision(2) << get_total_weight();

        return oss.str();
    }

private:
    int m_pack_number = 0;
    std::vector<item> m_items;
    int m_total_items = 0;
    double m_total_weight = 0.0;
    int m_max_length = 0;
};
