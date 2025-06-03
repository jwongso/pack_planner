#pragma once

#include <string>
#include <iomanip>

/**
 * @brief Represents an item with id, length, quantity, and weight properties
 */
class item {
public:
    /**
     * @brief Construct a new item object
     * @param id The item identifier
     * @param length The item length
     * @param quantity The item quantity
     * @param weight The weight per piece
     */
    item(int id, int length, int quantity, double weight) noexcept
    : m_id(id), m_length(length), m_quantity(quantity), m_weight(weight)
    {}

    // Getters
    /**
     * @brief Get the item ID
     * @return int The item ID
     */
    [[nodiscard]] int get_id() const noexcept { return m_id; }

    /**
     * @brief Get the item length
     * @return int The item length
     */
    [[nodiscard]] int get_length() const noexcept { return m_length; }

    /**
     * @brief Get the item quantity
     * @return int The item quantity
     */
    [[nodiscard]] int get_quantity() const noexcept { return m_quantity; }

    /**
     * @brief Get the weight per piece
     * @return double The weight per piece
     */
    [[nodiscard]] double get_weight() const noexcept { return m_weight; }

    // Setters
    /**
     * @brief Set the item quantity
     * @param quantity The new quantity
     */
    void set_quantity(int quantity) noexcept { m_quantity = quantity; }

    /**
     * @brief Get the total weight for this item (quantity * weight per piece)
     * @return double The total weight
     */
    [[nodiscard]] double get_total_weight() const noexcept { return m_quantity * m_weight; }

    /**
     * @brief Get string representation of the item
     * @return std::string The string representation
     */
    [[nodiscard]] std::string to_string() const {
        std::ostringstream oss;
        oss << m_id << "," << m_length << "," << m_quantity << "," << std::fixed << std::setprecision(3) << m_weight;
        return oss.str();
    }

    // Comparison operators for sorting
    /**
     * @brief Less than operator for sorting by length (short to long)
     * @param other The other item to compare with
     * @return bool True if this item's length is less than other's
     */
    [[nodiscard]] bool operator<(const item& other) const noexcept {
        // For sorting by length (short to long)
        return m_length < other.m_length;
    }

    /**
     * @brief Greater than operator for sorting by length (long to short)
     * @param other The other item to compare with
     * @return bool True if this item's length is greater than other's
     */
    [[nodiscard]] bool operator>(const item& other) const noexcept {
        // For sorting by length (long to short)
        return m_length > other.m_length;
    }

private:
    int m_id;
    int m_length;
    int m_quantity;
    double m_weight;  // weight per piece
};
