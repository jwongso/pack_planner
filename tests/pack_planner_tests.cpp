#include <gtest/gtest.h>
#include <vector>
#include <limits>
#include <algorithm>
#include <random>
#include <chrono>

#include "item.h"
#include "pack.h"
#include "pack_planner.h"
#include "sort_order.h"

// Pack Planner Tests
class PackPlannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for pack planner tests
        planner = pack_planner();

        // Create some test items
        items = {
            item(1, 100, 5, 2.0),  // total weight: 10.0
            item(2, 200, 3, 3.0),  // total weight: 9.0
            item(3, 300, 2, 5.0),  // total weight: 10.0
            item(4, 150, 4, 2.5)   // total weight: 10.0
        };

        // Default configuration
        config.order = sort_order::NATURAL;
        config.max_items_per_pack = 10;
        config.max_weight_per_pack = 25.0;
    }

    pack_planner planner;
    std::vector<item> items;
    pack_planner_config config;
};

TEST_F(PackPlannerTest, PlanPacksNaturalOrder) {
    auto result = planner.plan_packs(config, items);

    // Should create 2 packs with natural order
    EXPECT_EQ(result.packs.size(), 2);
    EXPECT_EQ(result.packs[0].get_total_items(), 9);
    EXPECT_DOUBLE_EQ(result.packs[0].get_total_weight(), 24.0);
    EXPECT_EQ(result.packs[1].get_total_items(), 5);
    EXPECT_DOUBLE_EQ(result.packs[1].get_total_weight(), 15.0);

    // Check total items
    EXPECT_EQ(result.total_items, 14); // 5+3+2+4

    // Check utilization
    EXPECT_NEAR(result.utilization_percent, 78.0, 0.1); // (29.0+10.0)/(25.0*2) * 100
}

TEST_F(PackPlannerTest, PlanPacksShortToLong) {
    config.order = sort_order::SHORT_TO_LONG;
    auto result = planner.plan_packs(config, items);

    // Check that items were sorted by length
    const auto& packs = result.packs;
    EXPECT_EQ(packs.size(), 2);

    // First pack should have shorter items
    const auto& first_pack_items = packs[0].get_items();
    EXPECT_EQ(first_pack_items[0].get_length(), 100);

    // Verify total items and weights
    EXPECT_EQ(result.total_items, 14);
}

TEST_F(PackPlannerTest, PlanPacksLongToShort) {
    config.order = sort_order::LONG_TO_SHORT;
    auto result = planner.plan_packs(config, items);

    // Check that items were sorted by length
    const auto& packs = result.packs;
    EXPECT_EQ(packs.size(), 2);

    // First pack should have longer items
    const auto& first_pack_items = packs[0].get_items();
    EXPECT_EQ(first_pack_items[0].get_length(), 300);

    // Verify total items and weights
    EXPECT_EQ(result.total_items, 14);
}

TEST_F(PackPlannerTest, PlanPacksEmptyItems) {
    std::vector<item> empty_items;
    auto result = planner.plan_packs(config, empty_items);

    EXPECT_EQ(result.packs.size(), 1); // Should create one empty pack
    EXPECT_TRUE(result.packs[0].is_empty());
    EXPECT_EQ(result.total_items, 0);
    EXPECT_DOUBLE_EQ(result.utilization_percent, 0.0);
}

TEST_F(PackPlannerTest, PlanPacksLargeQuantities) {
    // Create items with large quantities
    std::vector<item> large_items = {
        item(1, 100, 50, 1.0),  // 50 items, 50.0 weight
        item(2, 200, 30, 2.0)   // 30 items, 60.0 weight
    };

    auto result = planner.plan_packs(config, large_items);

    // Should create multiple packs
    EXPECT_GT(result.packs.size(), 7); // At least 8 packs needed

    // Check total items
    EXPECT_EQ(result.total_items, 80); // 50+30

    // Verify no pack exceeds constraints
    for (const auto& p : result.packs) {
        if (!p.is_empty()) {
            EXPECT_LE(p.get_total_items(), config.max_items_per_pack);
            EXPECT_LE(p.get_total_weight(), config.max_weight_per_pack);
        }
    }
}

TEST_F(PackPlannerTest, PlanPacksHeavyItems) {
    // Create items that are heavy
    std::vector<item> heavy_items = {
        item(1, 100, 1, 20.0),  // 1 item, 20.0 weight
        item(2, 200, 1, 15.0),  // 1 item, 15.0 weight
        item(3, 300, 1, 25.0)   // 1 item, 25.0 weight - exactly at weight limit
    };

    auto result = planner.plan_packs(config, heavy_items);

    // Should create 3 packs (one for each item)
    EXPECT_EQ(result.packs.size(), 3);

    // Check that each pack has one item
    EXPECT_EQ(result.packs[0].get_total_items(), 1);
    EXPECT_EQ(result.packs[1].get_total_items(), 1);
    EXPECT_EQ(result.packs[2].get_total_items(), 1);

    // Check weights
    EXPECT_DOUBLE_EQ(result.packs[0].get_total_weight(), 20.0);
    EXPECT_DOUBLE_EQ(result.packs[1].get_total_weight(), 15.0);
    EXPECT_DOUBLE_EQ(result.packs[2].get_total_weight(), 25.0);
}

TEST_F(PackPlannerTest, PlanPacksExtremelyHeavyItem) {
    // Create an item that exceeds max weight
    std::vector<item> extreme_items = {
        item(1, 100, 1, 30.0)  // 1 item, 30.0 weight (exceeds max_weight_per_pack)
    };

    auto result = planner.plan_packs(config, extreme_items);

    // Should create 1 pack but it will be empty because item can't fit
    EXPECT_EQ(result.packs.size(), 2);
    EXPECT_TRUE(result.packs[0].is_empty());
    EXPECT_EQ(result.total_items, 1); // Total items count includes all input items
    EXPECT_DOUBLE_EQ(result.utilization_percent, 0.0);
}

TEST_F(PackPlannerTest, CalculateUtilization) {
    // Create some packs with known weights
    std::vector<pack> test_packs;

    pack p1(1);
    int added = p1.add_partial_item(1, 100, 5, 2.0, 10, 25.0); // 10.0 weight

    pack p2(2);
    added = p2.add_partial_item(2, 200, 3, 5.0, 10, 25.0); // 15.0 weight

    pack p3(3); // Empty pack

    test_packs.push_back(p1);
    test_packs.push_back(p2);
    test_packs.push_back(p3);

    double utilization = planner.calculate_utilization(test_packs, 25.0);

    // Utilization = (10.0 + 15.0) / (25.0 * 2) * 100 = 50%
    EXPECT_DOUBLE_EQ(utilization, 50.0);
}

TEST_F(PackPlannerTest, CalculateUtilizationEmptyPacks) {
    std::vector<pack> empty_packs;
    double utilization = planner.calculate_utilization(empty_packs, 25.0);
    EXPECT_DOUBLE_EQ(utilization, 0.0);

    // Add one empty pack
    empty_packs.emplace_back(1);
    utilization = planner.calculate_utilization(empty_packs, 25.0);
    EXPECT_DOUBLE_EQ(utilization, 0.0);
}

TEST_F(PackPlannerTest, OutputResults) {
    // Create a pack with known content
    pack p1(1);
    int added = p1.add_partial_item(1, 100, 5, 2.0, 10, 25.0);

    std::vector<pack> test_packs = {p1};

    // Capture output
    std::stringstream output;
    planner.output_results(test_packs, output);

    std::string result = output.str();
    EXPECT_TRUE(result.find("Pack Number: 1") != std::string::npos);
    EXPECT_TRUE(result.find("1,100,5,2.000") != std::string::npos);
    EXPECT_TRUE(result.find("Pack Length: 100") != std::string::npos);
    EXPECT_TRUE(result.find("Pack Weight: 10.00") != std::string::npos);
}

// Performance and Edge Case Tests
TEST_F(PackPlannerTest, PerformanceTest) {
    // Create a large number of items
    std::vector<item> many_items;
    many_items.reserve(1000);

    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> length_dist(50, 500);
    std::uniform_int_distribution<int> quantity_dist(1, 5);
    std::uniform_real_distribution<double> weight_dist(0.5, 5.0);

    for (int i = 0; i < 1000; ++i) {
        many_items.emplace_back(
            i + 1,
            length_dist(rng),
            quantity_dist(rng),
            weight_dist(rng)
        );
    }

    // Measure time to plan packs
    auto start = std::chrono::high_resolution_clock::now();
    auto result = planner.plan_packs(config, many_items);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    // Just verify it completes in a reasonable time
    // This is not a strict test, but helps catch major performance regressions
    EXPECT_LT(elapsed.count(), 1000.0); // Should complete in less than 1 second

    // Verify all items were packed
    int total_packed = 0;
    for (const auto& p : result.packs) {
        total_packed += p.get_total_items();
    }

    int total_input = 0;
    for (const auto& i : many_items) {
        total_input += i.get_quantity();
    }

    EXPECT_EQ(total_packed, total_input);
}

TEST_F(PackPlannerTest, EdgeCaseZeroWeightItems) {
    // Create items with zero weight
    std::vector<item> zero_weight_items = {
        item(1, 100, 5, 0.0),
        item(2, 200, 10, 0.0)
    };

    auto result = planner.plan_packs(config, zero_weight_items);

    // Should be limited by max_items_per_pack
    EXPECT_EQ(result.packs.size(), 2);
    EXPECT_EQ(result.packs[0].get_total_items(), 10);
    EXPECT_EQ(result.packs[1].get_total_items(), 5);
    EXPECT_DOUBLE_EQ(result.packs[0].get_total_weight(), 0.0);
    EXPECT_DOUBLE_EQ(result.packs[1].get_total_weight(), 0.0);
}

TEST_F(PackPlannerTest, EdgeCaseZeroQuantityItems) {
    // Create items with zero quantity
    std::vector<item> zero_quantity_items = {
        item(1, 100, 0, 2.0),
        item(2, 200, 0, 3.0),
        item(3, 300, 5, 1.0)  // One normal item
    };

    auto result = planner.plan_packs(config, zero_quantity_items);

    // Should only pack the normal item
    EXPECT_EQ(result.packs.size(), 1);
    EXPECT_EQ(result.packs[0].get_total_items(), 5);
    EXPECT_DOUBLE_EQ(result.packs[0].get_total_weight(), 5.0);
}

TEST_F(PackPlannerTest, EdgeCaseNegativeValues) {
    // Create items with negative values (should be handled gracefully)
    std::vector<item> negative_items = {
        item(1, -100, 5, 2.0),    // Negative length
        item(2, 200, -3, 3.0),    // Negative quantity
        item(3, 300, 2, -5.0),    // Negative weight
        item(4, 150, 4, 2.5)      // Normal item
    };

    auto result = planner.plan_packs(config, negative_items);

    // Should handle negative values without crashing
    // Exact behavior depends on implementation, but should not crash
    EXPECT_GE(result.packs.size(), 1);
}

TEST_F(PackPlannerTest, EdgeCaseExtremeValues) {
    // Create items with extreme values
    std::vector<item> extreme_items = {
        item(1, std::numeric_limits<int>::max(), 1, 1.0),  // Max int length
        item(2, 100, std::numeric_limits<int>::max() / 1000, 0.001),  // Very large quantity
        item(3, 100, 1, std::numeric_limits<double>::min()),  // Min positive double weight
        item(4, 100, 1, std::numeric_limits<double>::max() / 1e20)  // Very large but not max double weight
    };

    // This test mainly checks that the code doesn't crash with extreme values
    auto result = planner.plan_packs(config, extreme_items);

    // Just verify it completes without crashing
    EXPECT_GE(result.packs.size(), 1);
}

// Main function to run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
