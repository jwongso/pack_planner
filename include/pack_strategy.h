#pragma once

#include <vector>
#include <memory>
#include "item.h"
#include "pack.h"

enum class strategy_type {
    BLOCKING_FIRST_FIT,
    PARALLEL_FIRST_FIT,
    LOCKFREE_FIRST_FIT,
    BLOCKING_NEXT_FIT
};

/**
 * @brief Strategy interface for different packing algorithms
 */
class pack_strategy {
public:
    virtual ~pack_strategy() = default;

    /**
     * @brief Pack items using the specific strategy
     * @param items Items to pack
     * @param max_items Maximum items per pack
     * @param max_weight Maximum weight per pack
     * @return std::vector<pack> Vector of packed items
     */
    virtual std::vector<pack> pack_items(const std::vector<item>& items,
                                       int max_items,
                                       double max_weight) = 0;

    /**
     * @brief Get strategy name for identification
     * @return std::string Strategy name
     */
    virtual std::string get_name() const = 0;
};

/**
 * @brief Factory for creating pack strategies
 */
class pack_strategy_factory {
public:
    /**
     * @brief Create a pack strategy
     * @param type Strategy type to create
     * @param thread_count Number of threads for parallel strategy (ignored for others)
     * @return std::unique_ptr<pack_strategy> Created strategy
     */
    static std::unique_ptr<pack_strategy> create_strategy(
        strategy_type type,
        int thread_count = 4);

    /**
     * @brief Parse strategy type from string
     * @param str String representation
     * @return strategy_type Parsed strategy type
     */
    static strategy_type parse_strategy_type(const std::string& str);

    /**
     * @brief Convert strategy type to string
     * @param type Strategy type
     * @return std::string String representation
     */
    static std::string strategy_type_to_string(strategy_type type);

    /**
     * @brief Get all available strategies
     * @return std::vector<strategy_type> List of all strategies
     */
    static std::vector<strategy_type> get_all_strategies();

    /**
     * @brief Get only fast strategies (excludes WORST_FIT, BEST_FIT)
     * @return std::vector<strategy_type> List of fast strategies
     */
    static std::vector<strategy_type> get_fast_strategies();

    /**
     * @brief Check if a strategy is parallel
     * @param type Strategy type
     * @return bool True if parallel strategy
     */
    static bool is_parallel_strategy(strategy_type type);

    /**
     * @brief Get default thread count for a strategy
     * @param type Strategy type
     * @return int Default thread count (1 for sequential, hardware_concurrency for parallel)
     */
    static int get_default_thread_count(strategy_type type);
};
