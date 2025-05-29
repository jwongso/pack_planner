#ifndef ITEM_H
#define ITEM_H

#include <stdexcept>

namespace pplanner {

// item class representing individual items
class item {
private:
    int m_id;
    int m_length;
    int m_quantity;
    double m_weight;

public:
    item(int id, int length, int quantity, double weight)
        : m_id(id), m_length(length), m_quantity(quantity), m_weight(weight) {
        if (quantity <= 0 || weight < 0 || length < 0) {
            throw std::invalid_argument("Invalid item parameters");
        }
    }

    // Getters
    int get_id() const { return m_id; }
    int get_length() const { return m_length; }
    int get_quantity() const { return m_quantity; }
    double get_weight() const { return m_weight; }
    double get_total_weight() const { return m_weight * m_quantity; }

    // Create a new item with specified quantity (for splitting)
    item with_quantity(int new_quantity) const {
        return item(m_id, m_length, new_quantity, m_weight);
    }

    // Comparison operators for sorting
    bool operator<(const item& other) const {
        return m_length < other.m_length;
    }

    bool operator>(const item& other) const {
        return m_length > other.m_length;
    }
};

} // pplanner

#endif // ITEM_H
