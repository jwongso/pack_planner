#include "Pack.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

Pack::Pack(int packNumber) : packNumber_(packNumber) {
    // Reserve some space to avoid initial reallocations
    items_.reserve(8);
}

bool Pack::addItem(const Item& item, int maxItems, double maxWeight) {
    int newQuantity = totalItems_ + item.getQuantity();
    double newWeight = totalWeight_ + item.getTotalWeight();

    if (newQuantity <= maxItems && newWeight <= maxWeight) {
        items_.push_back(item);
        totalItems_ = newQuantity;
        totalWeight_ = newWeight;
        maxLength_ = std::max(maxLength_, item.getLength());
        return true;
    }
    return false;
}

// Optimized version that takes the remaining quantity directly
int Pack::addPartialItem(const Item& item, int remainingQuantity, int maxItems, double maxWeight) {
    // Early exit if pack constraints are already exceeded
    if (totalItems_ >= maxItems || totalWeight_ >= maxWeight) {
        return 0;
    }

    int maxByItems = maxItems - totalItems_;

    // Only calculate weight constraint if we haven't already hit the item limit
    int canAdd = std::min(maxByItems, remainingQuantity);
    if (canAdd > 0) {
        // Calculate how many we can fit by weight
        double remainingWeight = maxWeight - totalWeight_;
        int maxByWeight = static_cast<int>(remainingWeight / item.getWeight());
        canAdd = std::min(canAdd, maxByWeight);
    }

    if (canAdd > 0) {
        items_.emplace_back(item.getId(), item.getLength(), canAdd, item.getWeight());
        totalItems_ += canAdd;
        totalWeight_ += canAdd * item.getWeight();
        maxLength_ = std::max(maxLength_, item.getLength());
    }

    return canAdd;
}

int Pack::addPartialItem(int id, int length, int quantity, double weight,
                         int maxItems, double maxWeight) {
    int maxByItems = maxItems - totalItems_;
    int maxByWeight = static_cast<int>((maxWeight - totalWeight_) / weight);
    int canAdd = std::min({maxByItems, maxByWeight, quantity});

    if (canAdd > 0) {
        items_.emplace_back(id, length, canAdd, weight);
        totalItems_ += canAdd;
        totalWeight_ += canAdd * weight;
        maxLength_ = std::max(maxLength_, length);
    }
    return canAdd;
}

bool Pack::isFull(int maxItems, double maxWeight) const {
    return totalItems_ >= maxItems || totalWeight_ >= maxWeight - 1e-9;
}

// Keep the old version for backward compatibility
int Pack::addPartialItem(const Item& item, int maxItems, double maxWeight) {
    return addPartialItem(item, item.getQuantity(), maxItems, maxWeight);
}

int Pack::getTotalItems() const {
    return totalItems_;
}

double Pack::getTotalWeight() const {
    return totalWeight_;
}

int Pack::getPackLength() const {
    return maxLength_;
}

std::string Pack::toString() const {
    std::ostringstream oss;
    oss << "Pack Number: " << packNumber_ << "\n";

    for (const auto& item : items_) {
        oss << item.toString() << "\n";
    }

    oss << "Pack Length: " << getPackLength()
        << ", Pack Weight: " << std::fixed << std::setprecision(2) << getTotalWeight();

    return oss.str();
}
