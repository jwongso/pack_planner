#include "pack_strategy.h"
#include "blocking_pack_strategy.h"
#include "parallel_pack_strategy.h"
#include "lockfree_pack_strategy.h"
#include "blocking_next_fit_strategy.h"

std::unique_ptr<pack_strategy> pack_strategy_factory::create_strategy(
    strategy_type type,
    int thread_count) {

    switch (type) {
        case strategy_type::BLOCKING_FIRST_FIT:
            return std::make_unique<blocking_pack_strategy>();

        case strategy_type::PARALLEL_FIRST_FIT:
            return std::make_unique<parallel_pack_strategy>(thread_count);

        case strategy_type::LOCKFREE_FIRST_FIT:
            return std::make_unique<lockfree_pack_strategy>(thread_count);

        case strategy_type::BLOCKING_NEXT_FIT:
            return std::make_unique<next_fit_pack_strategy>();

        default:
            return std::make_unique<blocking_pack_strategy>();
    }
}

strategy_type pack_strategy_factory::parse_strategy_type(const std::string& str) {
    if (str == "Parallel First Fit") return strategy_type::PARALLEL_FIRST_FIT;
    if (str == "Blocking First Fit") return strategy_type::BLOCKING_FIRST_FIT;
    if (str == "Lock-free First Fit") return strategy_type::LOCKFREE_FIRST_FIT;
    if (str == "Next-Fit") return strategy_type::BLOCKING_NEXT_FIT;
    return strategy_type::BLOCKING_FIRST_FIT; // default
}

std::string pack_strategy_factory::strategy_type_to_string(strategy_type type) {
    switch (type) {
        case strategy_type::BLOCKING_FIRST_FIT: return "Blocking First Fit";
        case strategy_type::PARALLEL_FIRST_FIT: return "Parallel First Fit";
        case strategy_type::LOCKFREE_FIRST_FIT: return "Lock-free First Fit";
        case strategy_type::BLOCKING_NEXT_FIT: return "Next-Fit";
        default: return "Blocking First Fit";
    }
}
