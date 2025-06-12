#include "pack_strategy.h"
#include "blocking_pack_strategy.h"
#include "parallel_pack_strategy.h"

std::unique_ptr<pack_strategy> pack_strategy_factory::create_strategy(
    strategy_type type,
    int thread_count) {

    switch (type) {
        case strategy_type::BLOCKING_FIRST_FIT:
            return std::make_unique<blocking_pack_strategy>();

        case strategy_type::PARALLEL_FIRST_FIT:
            return std::make_unique<parallel_pack_strategy>(thread_count);

        default:
            return std::make_unique<blocking_pack_strategy>();
    }
}

strategy_type pack_strategy_factory::parse_strategy_type(const std::string& str) {
    if (str == "Parallel First Fit") return strategy_type::PARALLEL_FIRST_FIT;
    if (str == "Blocking First Fit") return strategy_type::BLOCKING_FIRST_FIT;
    return strategy_type::BLOCKING_FIRST_FIT; // default
}

std::string pack_strategy_factory::strategy_type_to_string(strategy_type type) {
    switch (type) {
        case strategy_type::BLOCKING_FIRST_FIT: return "Blocking First Fit";
        case strategy_type::PARALLEL_FIRST_FIT: return "Parallel First Fit";
        default: return "Blocking First Fit";
    }
}
