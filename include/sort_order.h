#pragma once

#include <string>

/**
 * @brief Enumeration for different item sorting orders
 */
enum class sort_order {
    NATURAL,
    SHORT_TO_LONG,
    LONG_TO_SHORT
};

/**
 * @brief Parse a string to get the corresponding sort_order
 * @param str The string to parse
 * @return sort_order The parsed sort order
 */
[[nodiscard]] inline sort_order parse_sort_order(const std::string& str) noexcept {
    if (str == "NATURAL") return sort_order::NATURAL;
    if (str == "SHORT_TO_LONG") return sort_order::SHORT_TO_LONG;
    if (str == "LONG_TO_SHORT") return sort_order::LONG_TO_SHORT;
    return sort_order::NATURAL; // default
}

/**
 * @brief Convert a sort_order to its string representation
 * @param order The sort order to convert
 * @return std::string The string representation
 */
[[nodiscard]] inline std::string sort_order_to_string(sort_order order) noexcept {
    switch (order) {
        case sort_order::NATURAL: return "NAT";
        case sort_order::SHORT_TO_LONG: return "STL";
        case sort_order::LONG_TO_SHORT: return "LTS";
        default: return "NAT";
    }
}
