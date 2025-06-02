#pragma once

#include <vector>
#include <string>
#include "Item.h"

class Pack {
public:
    Pack(int packNumber);

    // Add item to pack (returns true if successful, false if constraints violated)
    bool addItem(const Item& item, int maxItems, double maxWeight);

    // Try to add partial quantity of an item (optimized version)
    int addPartialItem(const Item& item, int remainingQuantity, int maxItems, double maxWeight);

    // Try to add partial quantity of an item (backward compatibility)
    int addPartialItem(const Item& item, int maxItems, double maxWeight);

    int addPartialItem(int id, int length, int quantity, double weight,
                       int maxItems, double maxWeight);

    bool isFull(int maxItems, double maxWeight) const;

    // Getters
    int getPackNumber() const { return packNumber_; }
    const std::vector<Item>& getItems() const { return items_; }
    int getTotalItems() const;
    double getTotalWeight() const;
    int getPackLength() const;  // Maximum length among all items

    // Check if pack is empty
    bool isEmpty() const { return items_.empty(); }

    // String representation
    std::string toString() const;

private:
    int packNumber_;
    std::vector<Item> items_;
    int totalItems_ = 0;
    double totalWeight_ = 0.0;
    int maxLength_ = 0;
};
