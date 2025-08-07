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
#include "optimized_sort.h"

// Pack Planner Tests - Base class for both strategies
class PackPlannerTestBase : public ::testing::TestWithParam<strategy_type> {
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

        // Default configuration with strategy from parameter
        config.order = sort_order::NATURAL;
        config.max_items_per_pack = 10;
        config.max_weight_per_pack = 25.0;
        config.type = GetParam();
        config.thread_count = 4;
    }

    pack_planner planner;
    std::vector<item> items;
    pack_planner_config config;
};

// Legacy class for backward compatibility - non-parameterized
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

        // Default configuration - using blocking strategy for legacy tests
        config.order = sort_order::NATURAL;
        config.max_items_per_pack = 10;
        config.max_weight_per_pack = 25.0;
        config.type = strategy_type::BLOCKING_FIRST_FIT;
        config.thread_count = 4;
    }

    pack_planner planner;
    std::vector<item> items;
    pack_planner_config config;
};

class ParallelPackStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for parallel strategy tests
        planner = pack_planner();

        // Create some test items
        items = {
            item(1, 100, 50, 2.0),  // total weight: 100.0
            item(2, 200, 30, 3.0),  // total weight: 90.0
            item(3, 300, 20, 5.0),  // total weight: 100.0
            item(4, 150, 40, 2.5)   // total weight: 100.0
        };

        // Default configuration
        config.order = sort_order::NATURAL;
        config.max_items_per_pack = 10;
        config.max_weight_per_pack = 25.0;
        config.type = strategy_type::PARALLEL_FIRST_FIT;
        config.thread_count = 4;
    }

    pack_planner planner;
    std::vector<item> items;
    pack_planner_config config;
};

// New test class for sorting algorithms
class SortingAlgorithmTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test items with various lengths
        test_items = {
            item(1, 500, 1, 1.0),
            item(2, 100, 1, 1.0),
            item(3, 1000, 1, 1.0),
            item(4, 250, 1, 1.0),
            item(5, 750, 1, 1.0),
            item(6, 100, 1, 1.0),  // Duplicate length
            item(7, 1000, 1, 1.0), // Duplicate length
        };
    }

    std::vector<item> test_items;

    // Helper to verify sorting order
    bool is_sorted_by_length(const std::vector<item>& items, bool ascending) {
        for (size_t i = 1; i < items.size(); ++i) {
            if (ascending) {
                if (items[i-1].get_length() > items[i].get_length()) return false;
            } else {
                if (items[i-1].get_length() < items[i].get_length()) return false;
            }
        }
        return true;
    }
};

// New test class for performance comparison
class PerformanceComparisonTest : public ::testing::Test {
protected:
    std::vector<item> generate_random_items(size_t count) {
        std::vector<item> items;
        items.reserve(count);

        std::mt19937 rng(42); // Fixed seed for reproducibility
        std::uniform_int_distribution<int> length_dist(100, 10000);
        std::uniform_int_distribution<int> quantity_dist(1, 10);
        std::uniform_real_distribution<double> weight_dist(0.1, 50.0);

        for (size_t i = 0; i < count; ++i) {
            items.emplace_back(
                i + 1,
                length_dist(rng),
                quantity_dist(rng),
                weight_dist(rng)
                );
        }

        return items;
    }

    bool is_sorted_by_length(const std::vector<item>& items, bool ascending) {
        for (size_t i = 1; i < items.size(); ++i) {
            if (ascending) {
                if (items[i-1].get_length() > items[i].get_length()) return false;
            } else {
                if (items[i-1].get_length() < items[i].get_length()) return false;
            }
        }
        return true;
    }
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
    EXPECT_EQ(result.packs.size(), 1);
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

TEST_F(ParallelPackStrategyTest, BasicParallelPacking) {
    auto result = planner.plan_packs(config, items);

    // Verify strategy name
    EXPECT_TRUE(result.strategy_name.find("Parallel") != std::string::npos);

    // Verify all items were packed
    int total_input = 0;
    for (const auto& i : items) {
        total_input += i.get_quantity();
    }

    EXPECT_EQ(result.total_items, total_input);

    // Verify no pack exceeds constraints
    for (const auto& p : result.packs) {
        if (!p.is_empty()) {
            EXPECT_LE(p.get_total_items(), config.max_items_per_pack);
            EXPECT_LE(p.get_total_weight(), config.max_weight_per_pack);
        }
    }
}

TEST_F(ParallelPackStrategyTest, CompareWithBlockingStrategy) {
    // First run with parallel strategy
    auto parallel_result = planner.plan_packs(config, items);

    // Then run with blocking strategy
    config.type = strategy_type::BLOCKING_FIRST_FIT;
    auto blocking_result = planner.plan_packs(config, items);

    // Both strategies should pack all items
    EXPECT_EQ(parallel_result.total_items, blocking_result.total_items);

    // Compare utilization - should be similar (within 5%)
    EXPECT_NEAR(parallel_result.utilization_percent,
                blocking_result.utilization_percent,
                5.0);
}

TEST_F(ParallelPackStrategyTest, ThreadCountImpact) {
    // Test with different thread counts
    std::vector<int> thread_counts = {1, 2, 4, 8};
    std::vector<double> packing_times;

    for (int threads : thread_counts) {
        config.thread_count = threads;
        auto result = planner.plan_packs(config, items);
        packing_times.push_back(result.packing_time);

        // Verify all items were packed correctly
        EXPECT_EQ(result.total_items, 140); // 50+30+20+40
    }

    // This is not a strict test as performance depends on hardware
    // but generally more threads should not be drastically slower
    // Just check that the test runs without errors
    EXPECT_EQ(packing_times.size(), thread_counts.size());
}


// Parameterized tests for both strategies
TEST_P(PackPlannerTestBase, PlanPacksNaturalOrderBothStrategies) {
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

TEST_P(PackPlannerTestBase, PlanPacksShortToLongBothStrategies) {
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

TEST_P(PackPlannerTestBase, PlanPacksLongToShortBothStrategies) {
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

TEST_P(PackPlannerTestBase, PlanPacksEmptyItemsBothStrategies) {
    std::vector<item> empty_items;
    auto result = planner.plan_packs(config, empty_items);

    EXPECT_EQ(result.packs.size(), 1); // Should create one empty pack
    EXPECT_TRUE(result.packs[0].is_empty());
    EXPECT_EQ(result.total_items, 0);
    EXPECT_DOUBLE_EQ(result.utilization_percent, 0.0);
}

TEST_P(PackPlannerTestBase, PlanPacksLargeQuantitiesBothStrategies) {
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

TEST_P(PackPlannerTestBase, PlanPacksHeavyItemsBothStrategies) {
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

TEST_P(PackPlannerTestBase, PlanPacksExtremelyHeavyItemBothStrategies) {
    // Create an item that exceeds max weight
    std::vector<item> extreme_items = {
        item(1, 100, 1, 30.0)  // 1 item, 30.0 weight (exceeds max_weight_per_pack)
    };

    auto result = planner.plan_packs(config, extreme_items);

    // Should create 1 pack but it will be empty because item can't fit
    EXPECT_EQ(result.packs.size(), 1);
    EXPECT_TRUE(result.packs[0].is_empty());
    EXPECT_EQ(result.total_items, 1); // Total items count includes all input items
    EXPECT_DOUBLE_EQ(result.utilization_percent, 0.0);
}

TEST_P(PackPlannerTestBase, EdgeCaseZeroWeightItemsBothStrategies) {
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

TEST_P(PackPlannerTestBase, EdgeCaseZeroQuantityItemsBothStrategies) {
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

TEST_P(PackPlannerTestBase, EdgeCaseNegativeValuesBothStrategies) {
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

TEST_F(SortingAlgorithmTest, RadixSortAscending) {
    auto items_copy = test_items;
    optimized_sort::RadixSort::sort_by_length(items_copy, true);

    EXPECT_TRUE(is_sorted_by_length(items_copy, true));
    EXPECT_EQ(items_copy[0].get_length(), 100);
    EXPECT_EQ(items_copy.back().get_length(), 1000);
}

TEST_F(SortingAlgorithmTest, RadixSortDescending) {
    auto items_copy = test_items;
    optimized_sort::RadixSort::sort_by_length(items_copy, false);

    EXPECT_TRUE(is_sorted_by_length(items_copy, false));
    EXPECT_EQ(items_copy[0].get_length(), 1000);
    EXPECT_EQ(items_copy.back().get_length(), 100);
}

#ifdef __AVX2__
TEST_F(SortingAlgorithmTest, SIMDRadixSortAscending) {
    auto items_copy = test_items;
    optimized_sort::SIMDRadixSort::sort_by_length(items_copy, true);

    EXPECT_TRUE(is_sorted_by_length(items_copy, true));
    EXPECT_EQ(items_copy[0].get_length(), 100);
    EXPECT_EQ(items_copy.back().get_length(), 1000);
}

TEST_F(SortingAlgorithmTest, SIMDRadixSortDescending) {
    auto items_copy = test_items;
    optimized_sort::SIMDRadixSort::sort_by_length(items_copy, false);

    EXPECT_TRUE(is_sorted_by_length(items_copy, false));
    EXPECT_EQ(items_copy[0].get_length(), 1000);
    EXPECT_EQ(items_copy.back().get_length(), 100);
}

TEST_F(SortingAlgorithmTest, SIMDRadixSortV2Ascending) {
    auto items_copy = test_items;
    optimized_sort::SIMDRadixSortV2::sort_by_length(items_copy, true);

    EXPECT_TRUE(is_sorted_by_length(items_copy, true));
    EXPECT_EQ(items_copy[0].get_length(), 100);
    EXPECT_EQ(items_copy.back().get_length(), 1000);
}

TEST_F(SortingAlgorithmTest, SIMDRadixSortV2Descending) {
    auto items_copy = test_items;
    optimized_sort::SIMDRadixSortV2::sort_by_length(items_copy, false);

    EXPECT_TRUE(is_sorted_by_length(items_copy, false));
    EXPECT_EQ(items_copy[0].get_length(), 1000);
    EXPECT_EQ(items_copy.back().get_length(), 100);
}
#endif

TEST_F(SortingAlgorithmTest, CountingSortAscending) {
    auto items_copy = test_items;
    optimized_sort::CountingSort::sort_by_length(items_copy, true);

    EXPECT_TRUE(is_sorted_by_length(items_copy, true));
    EXPECT_EQ(items_copy[0].get_length(), 100);
    EXPECT_EQ(items_copy.back().get_length(), 1000);
}

TEST_F(SortingAlgorithmTest, CountingSortDescending) {
    auto items_copy = test_items;
    optimized_sort::CountingSort::sort_by_length(items_copy, false);

    EXPECT_TRUE(is_sorted_by_length(items_copy, false));
    EXPECT_EQ(items_copy[0].get_length(), 1000);
    EXPECT_EQ(items_copy.back().get_length(), 100);
}

TEST_F(SortingAlgorithmTest, EmptyVector) {
    std::vector<item> empty;

    // All sorting algorithms should handle empty vectors
    optimized_sort::RadixSort::sort_by_length(empty, true);
    EXPECT_TRUE(empty.empty());

#ifdef __AVX2__
    optimized_sort::SIMDRadixSort::sort_by_length(empty, true);
    EXPECT_TRUE(empty.empty());

    optimized_sort::SIMDRadixSortV2::sort_by_length(empty, true);
    EXPECT_TRUE(empty.empty());
#endif

    optimized_sort::CountingSort::sort_by_length(empty, true);
    EXPECT_TRUE(empty.empty());
}

TEST_F(SortingAlgorithmTest, SingleItem) {
    std::vector<item> single = { item(1, 100, 1, 1.0) };

    optimized_sort::RadixSort::sort_by_length(single, true);
    EXPECT_EQ(single.size(), 1);
    EXPECT_EQ(single[0].get_length(), 100);
}

TEST_F(SortingAlgorithmTest, AllSameLength) {
    std::vector<item> same_length;
    for (int i = 0; i < 10; ++i) {
        same_length.emplace_back(i, 500, 1, 1.0);
    }

    auto original = same_length;
    optimized_sort::RadixSort::sort_by_length(same_length, true);

    // Should maintain relative order (stable sort)
    EXPECT_EQ(same_length.size(), original.size());
    for (size_t i = 0; i < same_length.size(); ++i) {
        EXPECT_EQ(same_length[i].get_id(), original[i].get_id());
    }
}

// Performance comparison tests
TEST_F(PerformanceComparisonTest, CompareSmallDataset) {
    const size_t size = 1000;
    auto items = generate_random_items(size);

    // Warm-up runs to avoid cold cache effects
    auto warmup = items;
    std::sort(warmup.begin(), warmup.end());
    warmup = items;
    optimized_sort::RadixSort::sort_by_length(warmup, true);

    // Run multiple iterations for more stable results
    const int iterations = 100;

    // Test std::sort
    auto items_std = items;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        items_std = items;
        std::sort(items_std.begin(), items_std.end());
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto std_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Test RadixSort
    auto items_radix = items;
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        items_radix = items;
        optimized_sort::RadixSort::sort_by_length(items_radix, true);
    }
    end = std::chrono::high_resolution_clock::now();
    auto radix_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

#ifdef __AVX2__
    // Test SIMDRadixSortV2
    auto items_simd = items;
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        items_simd = items;
        optimized_sort::SIMDRadixSortV2::sort_by_length(items_simd, true);
    }
    end = std::chrono::high_resolution_clock::now();
    auto simd_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // For small datasets, we just verify they complete successfully
    // Performance can vary based on CPU cache state and other factors
    EXPECT_GT(simd_time, 0);
    EXPECT_GT(std_time, 0);

    // Log the results for information
    std::cout << "Small dataset (" << size << " items, " << iterations << " iterations):\n";
    std::cout << "  std::sort: " << std_time << " μs\n";
    std::cout << "  RadixSort: " << radix_time << " μs\n";
    std::cout << "  SIMDRadixSortV2: " << simd_time << " μs\n";
#endif

    // For small datasets, just verify they all complete successfully
    EXPECT_GT(radix_time, 0);
    EXPECT_GT(std_time, 0);

    // Verify all produce the same sorted result
    EXPECT_TRUE(is_sorted_by_length(items_std, true));
    EXPECT_TRUE(is_sorted_by_length(items_radix, true));
}

// Add a test for larger datasets where RadixSort should clearly win
TEST_F(PerformanceComparisonTest, CompareLargeDataset) {
    const size_t size = 100000;  // 100K items
    auto items = generate_random_items(size);

    // Warm-up
    auto warmup = items;
    std::sort(warmup.begin(), warmup.end());

    // Test std::sort
    auto items_std = items;
    auto start = std::chrono::high_resolution_clock::now();
    std::sort(items_std.begin(), items_std.end());
    auto end = std::chrono::high_resolution_clock::now();
    auto std_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Test RadixSort
    auto items_radix = items;
    start = std::chrono::high_resolution_clock::now();
    optimized_sort::RadixSort::sort_by_length(items_radix, true);
    end = std::chrono::high_resolution_clock::now();
    auto radix_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

#ifdef __AVX2__
    // Test SIMDRadixSortV2
    auto items_simd = items;
    start = std::chrono::high_resolution_clock::now();
    optimized_sort::SIMDRadixSortV2::sort_by_length(items_simd, true);
    end = std::chrono::high_resolution_clock::now();
    auto simd_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // For large datasets, SIMD should be fastest
    EXPECT_LT(simd_time, std_time);

    std::cout << "Large dataset (" << size << " items):\n";
    std::cout << "  std::sort: " << std_time << " μs\n";
    std::cout << "  RadixSort: " << radix_time << " μs\n";
    std::cout << "  SIMDRadixSortV2: " << simd_time << " μs\n";
#endif

    // For large datasets, RadixSort should be faster than std::sort
    EXPECT_LT(radix_time, std_time);
}

// Update the benchmark regression test to be more lenient
TEST_F(PerformanceComparisonTest, BenchmarkRegression) {
    const size_t size = 10000;
    auto items = generate_random_items(size);

    // Warm-up
    auto warmup = items;
    optimized_sort::RadixSort::sort_by_length(warmup, true);

    // Run multiple iterations
    const int iterations = 10;
    auto items_test = items;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        items_test = items;
        optimized_sort::RadixSort::sort_by_length(items_test, true);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Calculate average throughput
    double avg_duration_per_sort = total_duration / static_cast<double>(iterations);
    double items_per_second = (size * 1000000.0) / avg_duration_per_sort;

    // Expect at least 5M items/second for RadixSort (more conservative)
    // This accounts for various CPU types and system loads
    EXPECT_GT(items_per_second, 5000000.0);

    std::cout << "RadixSort throughput: " << (items_per_second / 1000000.0)
              << "M items/sec\n";
}

// Integration tests with pack planner
TEST_F(PackPlannerTest, PlanPacksWithOptimizedSorting) {
    // Generate larger dataset to see sorting impact
    std::vector<item> large_items;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> length_dist(100, 1000);

    for (int i = 0; i < 1000; ++i) {
        large_items.emplace_back(i, length_dist(rng), 1, 1.0);
    }

    // Test with different sort orders
    config.order = sort_order::SHORT_TO_LONG;
    auto result_stl = planner.plan_packs(config, large_items);

    config.order = sort_order::LONG_TO_SHORT;
    auto result_lts = planner.plan_packs(config, large_items);

    // Both should pack all items
    EXPECT_EQ(result_stl.total_items, 1000);
    EXPECT_EQ(result_lts.total_items, 1000);

    // Verify sorting time is recorded
    EXPECT_GT(result_stl.sorting_time, 0.0);
    EXPECT_GT(result_lts.sorting_time, 0.0);
}

// Stress tests for sorting algorithms
TEST_F(SortingAlgorithmTest, LargeRandomDataset) {
    const size_t size = 100000;
    std::vector<item> items;
    items.reserve(size);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> length_dist(1, 10000);

    for (size_t i = 0; i < size; ++i) {
        items.emplace_back(i, length_dist(rng), 1, 1.0);
    }

    auto items_copy = items;
    optimized_sort::RadixSort::sort_by_length(items_copy, true);

    EXPECT_TRUE(is_sorted_by_length(items_copy, true));
    EXPECT_EQ(items_copy.size(), size);
}

TEST_F(SortingAlgorithmTest, ExtremeLengthValues) {
    std::vector<item> extreme_items = {
        item(1, 0, 1, 1.0),                    // Zero length
        item(2, 1, 1, 1.0),                    // Min positive
        item(3, INT_MAX, 1, 1.0),              // Max int
        item(4, INT_MAX / 2, 1, 1.0),         // Large value
    };

    auto items_copy = extreme_items;
    optimized_sort::RadixSort::sort_by_length(items_copy, true);

    EXPECT_TRUE(is_sorted_by_length(items_copy, true));
    EXPECT_EQ(items_copy[0].get_length(), 0);
    EXPECT_EQ(items_copy.back().get_length(), INT_MAX);
}

// Stress test with extreme dataset
TEST_F(PackPlannerTest, StressTestLargeDataset) {
    const int num_items = 50000;
    std::vector<item> huge_items;
    huge_items.reserve(num_items);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> length_dist(50, 10000);
    std::uniform_int_distribution<int> quantity_dist(1, 5);
    std::uniform_real_distribution<double> weight_dist(0.1, 5.0);

    for (int i = 0; i < num_items; ++i) {
        huge_items.emplace_back(
            i,
            length_dist(rng),
            quantity_dist(rng),
            weight_dist(rng)
            );
    }

    config.order = sort_order::SHORT_TO_LONG;
    config.max_items_per_pack = 20;
    config.max_weight_per_pack = 50.0;

    auto start = std::chrono::high_resolution_clock::now();
    auto result = planner.plan_packs(config, huge_items);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration<double>(end - start);

    // Should complete in reasonable time
    EXPECT_LT(duration.count(), 5.0); // Less than 5 seconds

    // Verify all items were considered
    int total_input = 0;
    for (const auto& item : huge_items) {
        total_input += item.get_quantity();
    }
    EXPECT_EQ(result.total_items, total_input);
}

// Test sorting with identical lengths
TEST_F(SortingAlgorithmTest, IdenticalLengths) {
    std::vector<item> same_length_items;
    for (int i = 0; i < 1000; ++i) {
        same_length_items.emplace_back(i, 500, 1, 1.0);
    }

    auto items_copy = same_length_items;
    optimized_sort::RadixSort::sort_by_length(items_copy, true);

    // Should complete without crashing
    EXPECT_EQ(items_copy.size(), same_length_items.size());

    // All items should still have same length
    for (const auto& item : items_copy) {
        EXPECT_EQ(item.get_length(), 500);
    }
}

// Instantiate parameterized tests for both strategies
INSTANTIATE_TEST_SUITE_P(
    AllStrategies,
    PackPlannerTestBase,
    ::testing::Values(
        strategy_type::BLOCKING_FIRST_FIT,
        strategy_type::PARALLEL_FIRST_FIT,
        strategy_type::LOCKFREE_FIRST_FIT
        ),
    [](const ::testing::TestParamInfo<strategy_type>& info) {
        switch (info.param) {
        case strategy_type::BLOCKING_FIRST_FIT:
            return "Blocking";
        case strategy_type::PARALLEL_FIRST_FIT:
            return "Parallel";
        case strategy_type::LOCKFREE_FIRST_FIT:
            return "LockFree";
        default:
            return "Unknown";
        }
    }
    );

// Main function to run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
