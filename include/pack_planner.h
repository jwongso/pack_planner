#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include "item.h"
#include "pack.h"
#include "sort_order.h"
#include "pack_strategy.h"
#include "timer.h"
#include "optimized_sort.h"

/**
 * @brief Configuration for the pack planning process
 */
struct pack_planner_config {
    sort_order order = sort_order::NATURAL;
    int max_items_per_pack = 100;
    double max_weight_per_pack = 200.0;
    strategy_type type = strategy_type::BLOCKING_FIRST_FIT;
    int thread_count = 4;

    // C++20: default all comparisons
    auto operator<=>(const pack_planner_config&) const = default;
};

/**
 * @brief Results of the pack planning process
 */
struct pack_planner_result {
    std::vector<pack> packs;
    double sorting_time;
    double packing_time;
    double total_time;
    int total_items;
    double utilization_percent;
    std::string strategy_name;
};

/**
 * @brief Class for planning how to pack items into packs
 */
class pack_planner {
public:
    /**
     * @brief Construct a new Pack Planner object
     */
    pack_planner() noexcept
        : m_strategy(pack_strategy_factory::create_strategy(strategy_type::BLOCKING_FIRST_FIT)) {}

    /**
     * @brief Plan packs with given configuration and items
     * @param config Configuration for planning
     * @param items Items to pack
     * @return pack_planner_result Results of the planning process
     */
    [[nodiscard]] pack_planner_result plan_packs(const pack_planner_config& config,
                                                std::vector<item> items) {
        pack_planner_result result;
        m_timer.start();

        // SAFETY: Validate and sanitize configuration
        pack_planner_config safe_config = config;
        safe_config.max_items_per_pack = std::max(1, config.max_items_per_pack);
        safe_config.max_weight_per_pack = std::max(0.1, config.max_weight_per_pack);
        safe_config.thread_count = std::clamp(config.thread_count, 1, 32);

        // Sort items
        timer sort_timer;
        sort_timer.start();
        sort_items(items, safe_config.order);
        result.sorting_time = sort_timer.stop();

        // Create or reuse strategy if config changed
        if (!m_strategy || config != m_config) {
            m_strategy = pack_strategy_factory::create_strategy(safe_config.type, safe_config.thread_count);
            m_config = safe_config;
        }

        result.strategy_name = m_strategy->get_name();

        // Pack
        timer pack_timer;
        pack_timer.start();
        result.packs = m_strategy->pack_items(items, safe_config.max_items_per_pack, safe_config.max_weight_per_pack);
        result.packing_time = pack_timer.stop();

        result.total_time = m_timer.stop();

        // SAFETY: Calculate total items safely
        result.total_items = 0;
        for (const auto& i : items) {
            // SAFETY: Skip negative quantities and avoid overflow
            if (i.get_quantity() > 0 &&
                result.total_items <= std::numeric_limits<int>::max() - i.get_quantity()) {
                result.total_items += i.get_quantity();
            }
        }

        result.utilization_percent = calculate_utilization(result.packs, safe_config.max_weight_per_pack);

        return result;
    }

    /**
     * @brief Output results to a stream
     * @param packs Packs to output
     * @param output Output stream (defaults to std::cout)
     */
    void output_results(const std::vector<pack>& packs, std::ostream& output = std::cout) const {
        for (const auto& p : packs) {
            if (!p.is_empty()) {
                output << p.to_string() << std::endl;
            }
        }
    }

    /**
     * @brief Calculate utilization percentage
     * @param packs Packs to calculate utilization for
     * @param max_weight Maximum weight per pack
     * @return double Utilization percentage
     */
    [[nodiscard]] double calculate_utilization(const std::vector<pack>& packs,
                                            double max_weight) const noexcept {
        if (packs.empty() || max_weight <= 0.0) return 0.0;

        double total_weight = 0.0;
        int non_empty_packs = 0;

        for (const auto& p : packs) {
            if (!p.is_empty()) {
                // SAFETY: Avoid potential floating-point overflow
                if (p.get_total_weight() >= 0.0 &&
                    total_weight <= std::numeric_limits<double>::max() - p.get_total_weight()) {
                    total_weight += p.get_total_weight();
                    non_empty_packs++;
                }
            }
        }

        if (non_empty_packs == 0) return 0.0;

        double max_possible_weight = non_empty_packs * max_weight;

        // SAFETY: Avoid division by zero
        if (max_possible_weight <= 0.0) return 0.0;

        // SAFETY: Clamp result to valid percentage range
        return std::clamp((total_weight / max_possible_weight) * 100.0, 0.0, 100.0);
    }

private:
    /**
     * @brief Sort items according to sort order
     * @param items Items to sort
     * @param order Sort order to use
     */
    void sort_items(std::vector<item>& items, sort_order order) noexcept {
        switch (order) {
        case sort_order::SHORT_TO_LONG:
#ifdef __AVX2__
            // Use SIMD-optimized RadixSort for best performance
            optimized_sort::SIMDRadixSortV2::sort_by_length(items, true);
#else
            // Fall back to regular RadixSort
            optimized_sort::RadixSort::sort_by_length(items, true);
#endif
            break;
        case sort_order::LONG_TO_SHORT:
#ifdef __AVX2__
            // Use SIMD-optimized RadixSort for best performance
            optimized_sort::SIMDRadixSortV2::sort_by_length(items, false);
#else
            // Fall back to regular RadixSort
            optimized_sort::RadixSort::sort_by_length(items, false);
#endif
        case sort_order::NATURAL:
        default:
            // Keep original order
            break;
        }
    }

    timer m_timer;
    std::unique_ptr<pack_strategy> m_strategy;
    pack_planner_config m_config{};
};
