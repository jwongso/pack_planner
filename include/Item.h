#pragma once

#include <string>

class Item {
public:
    Item(int id, int length, int quantity, double weight);
    
    // Getters
    int getId() const { return id_; }
    int getLength() const { return length_; }
    int getQuantity() const { return quantity_; }
    double getWeight() const { return weight_; }
    
    // Setters
    void setQuantity(int quantity) { quantity_ = quantity; }
    
    // Total weight for this item (quantity * weight per piece)
    double getTotalWeight() const { return quantity_ * weight_; }
    
    // String representation
    std::string toString() const;
    
    // Comparison operators for sorting
    bool operator<(const Item& other) const;
    bool operator>(const Item& other) const;

private:
    int id_;
    int length_;
    int quantity_;
    double weight_;  // weight per piece
};
