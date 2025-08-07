#include "pack_strategy.h"
#include "blocking_pack_strategy.h"
#include "parallel_pack_strategy.h"
#include "lockfree_pack_strategy.h"
#include "blocking_next_fit_strategy.h"

#include <algorithm>
#include <cctype>

std::unique_ptr<pack_strategy> pack_strategy_factory::create_strategy(
    strategy_type type,
    int thread_count) {

    switch (type) {
    case strategy_type::BLOCKING_FIRST_FIT:
        return std::make_unique<blocking_pack_strategy>();

    case strategy_type::BLOCKING_NEXT_FIT:
        return std::make_unique<next_fit_pack_strategy>();

    case strategy_type::PARALLEL_FIRST_FIT:
        return std::make_unique<parallel_pack_strategy>(thread_count);

    case strategy_type::LOCKFREE_FIRST_FIT:
        return std::make_unique<lockfree_pack_strategy>(thread_count);

    default:
        // Default to blocking next-fit (fastest)
        return std::make_unique<next_fit_pack_strategy>();
    }
}

strategy_type pack_strategy_factory::parse_strategy_type(const std::string& str) {
    // Convert to lowercase for case-insensitive comparison
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    // Check for various string representations
    if (lower_str == "blocking" || lower_str == "blocking_first_fit" ||
        lower_str == "first_fit" || lower_str == "firstfit") {
        return strategy_type::BLOCKING_FIRST_FIT;
    }

    if (lower_str == "next_fit" || lower_str == "nextfit" ||
        lower_str == "next-fit" || lower_str == "blocking_next_fit") {
        return strategy_type::BLOCKING_NEXT_FIT;
    }

    if (lower_str == "parallel" || lower_str == "parallel_first_fit" ||
        lower_str == "parallel-first-fit") {
        return strategy_type::PARALLEL_FIRST_FIT;
    }

    if (lower_str == "lockfree" || lower_str == "lock-free" ||
        lower_str == "lock_free" || lower_str == "lockfree_first_fit") {
        return strategy_type::LOCKFREE_FIRST_FIT;
    }

    // Default to next-fit (fastest)
    return strategy_type::BLOCKING_NEXT_FIT;
}

std::string pack_strategy_factory::strategy_type_to_string(strategy_type type) {
    switch (type) {
    case strategy_type::BLOCKING_FIRST_FIT:
        return "Blocking";

    case strategy_type::BLOCKING_NEXT_FIT:
        return "Next-Fit";

    case strategy_type::PARALLEL_FIRST_FIT:
        return "Parallel";

    case strategy_type::LOCKFREE_FIRST_FIT:
        return "Lock-free";

    default:
        return "Unknown";
    }
}

std::vector<strategy_type> pack_strategy_factory::get_all_strategies() {
    return {
        strategy_type::BLOCKING_FIRST_FIT,
        strategy_type::BLOCKING_NEXT_FIT,
        strategy_type::PARALLEL_FIRST_FIT,
        strategy_type::LOCKFREE_FIRST_FIT
    };
}

std::vector<strategy_type> pack_strategy_factory::get_fast_strategies() {
    // All remaining strategies are fast
    return get_all_strategies();
}

bool pack_strategy_factory::is_parallel_strategy(strategy_type type) {
    switch (type) {
    case strategy_type::PARALLEL_FIRST_FIT:
    case strategy_type::LOCKFREE_FIRST_FIT:
        return true;
    default:
        return false;
    }
}

int pack_strategy_factory::get_default_thread_count(strategy_type type) {
    if (is_parallel_strategy(type)) {
        return std::thread::hardware_concurrency();
    }
    return 1;
}
