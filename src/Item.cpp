#include "Item.h"
#include <sstream>
#include <iomanip>

Item::Item(int id, int length, int quantity, double weight)
    : id_(id), length_(length), quantity_(quantity), weight_(weight) {
}

std::string Item::toString() const {
    std::ostringstream oss;
    oss << id_ << "," << length_ << "," << quantity_ << "," << std::fixed << std::setprecision(3) << weight_;
    return oss.str();
}

bool Item::operator<(const Item& other) const {
    // For sorting by length (short to long)
    return length_ < other.length_;
}

bool Item::operator>(const Item& other) const {
    // For sorting by length (long to short)
    return length_ > other.length_;
}
