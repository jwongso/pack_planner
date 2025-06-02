#pragma once

#include <string>

enum class SortOrder {
    NATURAL,
    SHORT_TO_LONG,
    LONG_TO_SHORT
};

inline SortOrder parseSortOrder(const std::string& str) {
    if (str == "NATURAL") return SortOrder::NATURAL;
    if (str == "SHORT_TO_LONG") return SortOrder::SHORT_TO_LONG;
    if (str == "LONG_TO_SHORT") return SortOrder::LONG_TO_SHORT;
    return SortOrder::NATURAL; // default
}

inline std::string sortOrderToString(SortOrder order) {
    switch (order) {
        case SortOrder::NATURAL: return "NAT";
        case SortOrder::SHORT_TO_LONG: return "STL";
        case SortOrder::LONG_TO_SHORT: return "LTS";
        default: return "NAT";
    }
}
