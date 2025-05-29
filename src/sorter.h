#ifndef SORTER_H
#define SORTER_H

#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include "item.h"

namespace pplanner {

// Enums for better type safety
enum class sort_order {
    NATURAL,
    SHORT_TO_LONG,
    LONG_TO_SHORT
};

// Strategy pattern for sorting items
class item_sorter {
public:
    virtual ~item_sorter() = default;
    virtual void sort(std::vector<item>& items) const = 0;
};

class natural_sorter : public item_sorter {
public:
    void sort(std::vector<item>& /*items*/) const override {
        // Keep natural order - no sorting needed
    }
};

class short_to_long_sorter : public item_sorter {
public:
    void sort(std::vector<item>& items) const override {
        std::sort(items.begin(), items.end());
    }
};

class long_to_short_sorter : public item_sorter {
public:
    void sort(std::vector<item>& items) const override {
        std::sort(items.begin(), items.end(), std::greater<item>());
    }
};

// Factory for creating sorters
class sorter_factory {
public:
    static std::unique_ptr<item_sorter> create_sorter(sort_order order) {
        switch (order) {
            case sort_order::NATURAL:
                return std::make_unique<natural_sorter>();
            case sort_order::SHORT_TO_LONG:
                return std::make_unique<short_to_long_sorter>();
            case sort_order::LONG_TO_SHORT:
                return std::make_unique<long_to_short_sorter>();
            default:
                throw std::invalid_argument("Unknown sort order");
        }
    }
};

} // pplanner

#endif // SORTER_H
