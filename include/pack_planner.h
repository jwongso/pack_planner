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

/**
 * @brief Configuration for the pack planning process
 */
struct pack_planner_config {
    sort_order order = sort_order::NATURAL;
    int max_items_per_pack = 100;
    double max_weight_per_pack = 200.0;
    pack_strategy_factory::strategy_type strategy_type = pack_strategy_factory::strategy_type::BLOCKING;
    int thread_count = 4; // Used for parallel strategy
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
    pack_planner() noexcept {}

    /**
     * @brief Plan packs with given configuration and items
     * @param config Configuration for planning
     * @param items Items to pack
     * @return pack_planner_result Results of the planning process
     */
    [[nodiscard]] pack_planner_result plan_packs(const pack_planner_config& config,
                                                 std::vector<item> items) {
        pack_planner_result result;

        // Start total timing
        m_timer.start();

        // Sort items
        timer sort_timer;
        sort_timer.start();
        sort_items(items, config.order);
        result.sorting_time = sort_timer.stop();

        // Create strategy
        auto strategy = pack_strategy_factory::create_strategy(
            config.strategy_type, config.thread_count);
        result.strategy_name = strategy->get_name();

        // Pack items using selected strategy
        timer pack_timer;
        pack_timer.start();
        result.packs = strategy->pack_items(items, config.max_items_per_pack, config.max_weight_per_pack);
        result.packing_time = pack_timer.stop();

        result.total_time = m_timer.stop();

        // Calculate total items
        result.total_items = 0;
        for (const auto& i : items) {
            result.total_items += i.get_quantity();
        }

        // Calculate utilization
        result.utilization_percent = calculate_utilization(result.packs,
                                                           config.max_weight_per_pack);

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
        if (packs.empty()) return 0.0;

        double total_weight = 0.0;
        int non_empty_packs = 0;

        for (const auto& p : packs) {
            if (!p.is_empty()) {
                total_weight += p.get_total_weight();
                non_empty_packs++;
            }
        }

        if (non_empty_packs == 0) return 0.0;

        double max_possible_weight = non_empty_packs * max_weight;
        return (total_weight / max_possible_weight) * 100.0;
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
                std::sort(items.begin(), items.end());
                break;
            case sort_order::LONG_TO_SHORT:
                std::sort(items.begin(), items.end(), std::greater<item>());
                break;
            case sort_order::NATURAL:
            default:
                // Keep original order
                break;
        }
    }

    timer m_timer;
};
