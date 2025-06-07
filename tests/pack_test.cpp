#include <gtest/gtest.h>
#include "pack.h"

// Pack Class Tests
class PackTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for pack tests
        pack1 = pack(1);
        item1 = item(1, 100, 5, 2.0);  // total weight: 10.0
        item2 = item(2, 200, 3, 3.0);  // total weight: 9.0
        item3 = item(3, 300, 2, 5.0);  // total weight: 10.0
    }

    pack pack1{0};
    item item1{0, 0, 0, 0.0};
    item item2{0, 0, 0, 0.0};
    item item3{0, 0, 0, 0.0};

    const int default_max_items = 20;
    const double default_max_weight = 50.0;
};

TEST_F(PackTest, Constructor) {
    EXPECT_EQ(pack1.get_pack_number(), 1);
    EXPECT_TRUE(pack1.is_empty());
    EXPECT_EQ(pack1.get_total_items(), 0);
    EXPECT_DOUBLE_EQ(pack1.get_total_weight(), 0.0);
    EXPECT_EQ(pack1.get_pack_length(), 0);
}

TEST_F(PackTest, AddItem) {
    // Add first item
    EXPECT_TRUE(pack1.add_item(item1, default_max_items, default_max_weight));
    EXPECT_FALSE(pack1.is_empty());
    EXPECT_EQ(pack1.get_total_items(), 5);
    EXPECT_DOUBLE_EQ(pack1.get_total_weight(), 10.0);
    EXPECT_EQ(pack1.get_pack_length(), 100);

    // Add second item
    EXPECT_TRUE(pack1.add_item(item2, default_max_items, default_max_weight));
    EXPECT_EQ(pack1.get_total_items(), 8);
    EXPECT_DOUBLE_EQ(pack1.get_total_weight(), 19.0);
    EXPECT_EQ(pack1.get_pack_length(), 200);

    // Add third item
    EXPECT_TRUE(pack1.add_item(item3, default_max_items, default_max_weight));
    EXPECT_EQ(pack1.get_total_items(), 10);
    EXPECT_DOUBLE_EQ(pack1.get_total_weight(), 29.0);
    EXPECT_EQ(pack1.get_pack_length(), 300);
}

TEST_F(PackTest, AddItemExceedingMaxItems) {
    // Create an item with quantity that exceeds max_items
    item large_quantity(4, 100, 25, 1.0);

    // Should fail because it exceeds max_items
    EXPECT_FALSE(pack1.add_item(large_quantity, default_max_items, default_max_weight));
    EXPECT_TRUE(pack1.is_empty());

    // Add some items first
    EXPECT_TRUE(pack1.add_item(item1, default_max_items, default_max_weight)); // 5 items

    // Try to add an item that would exceed max_items
    item another_large(5, 100, 16, 1.0);
    EXPECT_FALSE(pack1.add_item(another_large, default_max_items, default_max_weight));
    EXPECT_EQ(pack1.get_total_items(), 5); // Should remain unchanged
}

TEST_F(PackTest, AddItemExceedingMaxWeight) {
    // Create an item with weight that exceeds max_weight
    item heavy_item(4, 100, 1, 60.0);

    // Should fail because it exceeds max_weight
    EXPECT_FALSE(pack1.add_item(heavy_item, default_max_items, default_max_weight));
    EXPECT_TRUE(pack1.is_empty());

    // Add some items first
    EXPECT_TRUE(pack1.add_item(item1, default_max_items, default_max_weight)); // 10.0 weight

    // Try to add an item that would exceed max_weight
    item another_heavy(5, 100, 5, 10.0); // 50.0 weight
    EXPECT_FALSE(pack1.add_item(another_heavy, default_max_items, default_max_weight));
    EXPECT_DOUBLE_EQ(pack1.get_total_weight(), 10.0); // Should remain unchanged
}

TEST_F(PackTest, AddPartialItem) {
    // Test adding partial quantity
    int added = pack1.add_partial_item(item1, default_max_items, default_max_weight);
    EXPECT_EQ(added, 5); // Should add all 5
    EXPECT_EQ(pack1.get_total_items(), 5);

    // Test adding partial quantity with remaining_quantity parameter
    item large_quantity(4, 100, 30, 1.0);
    added = pack1.add_partial_item(large_quantity, default_max_items, default_max_weight);
    EXPECT_EQ(added, 15); // Should add only 10 out of requested 30
    EXPECT_EQ(pack1.get_total_items(), 20);

    // Test adding partial quantity with individual parameters
    added = pack1.add_partial_item(5, 150, 10, 2.0, default_max_items, default_max_weight);
    EXPECT_EQ(added, 0); // Should add only 5 to reach max_items
    EXPECT_EQ(pack1.get_total_items(), 20);
    EXPECT_EQ(pack1.get_pack_length(), 100);
}

TEST_F(PackTest, AddPartialItemWeightConstraint) {
    // Fill pack to near weight limit
    EXPECT_TRUE(pack1.add_item(item1, default_max_items, default_max_weight)); // 10.0 weight
    EXPECT_TRUE(pack1.add_item(item2, default_max_items, default_max_weight)); // +9.0 = 19.0 weight
    EXPECT_TRUE(pack1.add_item(item3, default_max_items, default_max_weight)); // +10.0 = 29.0 weight

    // Try to add an item that would partially fit by weight
    item heavy_item(4, 100, 10, 3.0); // Each piece is 3.0 weight
    int added = pack1.add_partial_item(heavy_item, default_max_items, default_max_weight);

    // Should only add 7 pieces (7*3.0 = 21.0, bringing total to 50.0)
    EXPECT_EQ(added, 7);
    EXPECT_EQ(pack1.get_total_items(), 17); // 5+3+2+7
    EXPECT_DOUBLE_EQ(pack1.get_total_weight(), 50.0);
}

TEST_F(PackTest, AddPartialItemZeroCase) {
    // Fill pack to max items
    item small_item(4, 50, default_max_items, 1.0);
    EXPECT_TRUE(pack1.add_partial_item(small_item, default_max_items, default_max_weight));

    // Try to add more items when already full
    int added = pack1.add_partial_item(item1, default_max_items, default_max_weight);
    EXPECT_EQ(added, 0);

    // Create a new pack for weight test
    pack pack2(2);

    // Fill pack to max weight
    item weight_filler(5, 50, 5, 10.0); // 50.0 weight total
    added = pack2.add_partial_item(weight_filler, default_max_items, default_max_weight);

    // Try to add more items when already at weight limit
    added = pack2.add_partial_item(item1, default_max_items, default_max_weight);
    EXPECT_EQ(added, 0);
}

TEST_F(PackTest, IsFull) {
    EXPECT_FALSE(pack1.is_full(default_max_items, default_max_weight));

    // Fill to max items
    item filler(4, 50, default_max_items, 1.0);
    int added = pack1.add_partial_item(filler, default_max_items, default_max_weight);
    EXPECT_TRUE(pack1.is_full(default_max_items, default_max_weight));

    // Test with weight
    pack pack2(2);
    EXPECT_FALSE(pack2.is_full(default_max_items, default_max_weight));

    // Fill to max weight
    item weight_filler(5, 50, 5, 10.0); // 50.0 weight total
    added = pack2.add_partial_item(weight_filler, default_max_items, default_max_weight);
    EXPECT_TRUE(pack2.is_full(default_max_items, default_max_weight));

    // Test with almost full (weight)
    pack pack3(3);
    item almost_full(6, 50, 1, 49.99);
    EXPECT_TRUE(pack3.add_item(almost_full, default_max_items, default_max_weight));
    EXPECT_FALSE(pack3.is_full(default_max_items, default_max_weight)); // Should be considered full due to epsilon
}

TEST_F(PackTest, GetItems) {
    EXPECT_TRUE(pack1.add_item(item1, default_max_items, default_max_weight));
    EXPECT_TRUE(pack1.add_item(item2, default_max_items, default_max_weight));

    const auto& items = pack1.get_items();
    EXPECT_EQ(items.size(), 2);
    EXPECT_EQ(items[0].get_id(), 1);
    EXPECT_EQ(items[1].get_id(), 2);
}

TEST_F(PackTest, ToString) {
    EXPECT_TRUE(pack1.add_item(item1, default_max_items, default_max_weight));

    std::string result = pack1.to_string();
    EXPECT_TRUE(result.find("Pack Number: 1") != std::string::npos);
    EXPECT_TRUE(result.find("1,100,5,2.000") != std::string::npos);
    EXPECT_TRUE(result.find("Pack Length: 100") != std::string::npos);
    EXPECT_TRUE(result.find("Pack Weight: 10.00") != std::string::npos);
}
