#include <gtest/gtest.h>
#include "item.h"

// Item Class Tests
class ItemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for item tests
        basic_item = item(1, 100, 5, 2.5);
    }

    item basic_item{0, 0, 0, 0.0};
};

TEST_F(ItemTest, ConstructorAndGetters) {
    EXPECT_EQ(basic_item.get_id(), 1);
    EXPECT_EQ(basic_item.get_length(), 100);
    EXPECT_EQ(basic_item.get_quantity(), 5);
    EXPECT_DOUBLE_EQ(basic_item.get_weight(), 2.5);
}

TEST_F(ItemTest, SetQuantity) {
    basic_item.set_quantity(10);
    EXPECT_EQ(basic_item.get_quantity(), 10);

    // Test with zero
    basic_item.set_quantity(0);
    EXPECT_EQ(basic_item.get_quantity(), 0);

    // Test with negative (implementation should handle this gracefully)
    basic_item.set_quantity(-5);
    EXPECT_EQ(basic_item.get_quantity(), -5);
}

TEST_F(ItemTest, GetTotalWeight) {
    EXPECT_DOUBLE_EQ(basic_item.get_total_weight(), 12.5); // 5 * 2.5

    // Test with zero quantity
    basic_item.set_quantity(0);
    EXPECT_DOUBLE_EQ(basic_item.get_total_weight(), 0.0);

    // Test with large values
    basic_item.set_quantity(1000000);
    EXPECT_DOUBLE_EQ(basic_item.get_total_weight(), 1000000 * 2.5);
}

TEST_F(ItemTest, ToString) {
    std::string expected = "1,100,5,2.500";
    EXPECT_EQ(basic_item.to_string(), expected);

    // Test with different values
    item zero_item(0, 0, 0, 0.0);
    EXPECT_EQ(zero_item.to_string(), "0,0,0,0.000");

    // Test with negative values
    item negative_item(-1, -100, -5, -2.5);
    EXPECT_EQ(negative_item.to_string(), "-1,-100,-5,-2.500");
}

TEST_F(ItemTest, ComparisonOperators) {
    item shorter(2, 50, 5, 2.5);
    item longer(3, 150, 5, 2.5);
    item same_length(4, 100, 10, 5.0);

    // Less than operator (compares length)
    EXPECT_TRUE(shorter < basic_item);
    EXPECT_FALSE(basic_item < shorter);
    EXPECT_TRUE(basic_item < longer);
    EXPECT_FALSE(longer < basic_item);
    EXPECT_FALSE(basic_item < same_length);
    EXPECT_FALSE(same_length < basic_item);

    // Greater than operator (compares length)
    EXPECT_FALSE(shorter > basic_item);
    EXPECT_TRUE(basic_item > shorter);
    EXPECT_FALSE(basic_item > longer);
    EXPECT_TRUE(longer > basic_item);
    EXPECT_FALSE(basic_item > same_length);
    EXPECT_FALSE(same_length > basic_item);
}
