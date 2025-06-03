#include "pack_strategy.h"
#include "blocking_pack_strategy.h"
#include "parallel_pack_strategy.h"
#include "async_pack_strategy.h"

std::unique_ptr<pack_strategy> pack_strategy_factory::create_strategy(
    strategy_type type,
    int thread_count) {

    switch (type) {
        case strategy_type::BLOCKING:
            return std::make_unique<blocking_pack_strategy>();

        case strategy_type::PARALLEL:
            return std::make_unique<parallel_pack_strategy>(thread_count);

        case strategy_type::ASYNC:
            return std::make_unique<async_pack_strategy>();

        default:
            return std::make_unique<blocking_pack_strategy>();
    }
}

pack_strategy_factory::strategy_type pack_strategy_factory::parse_strategy_type(const std::string& str) {
    if (str == "PARALLEL") return strategy_type::PARALLEL;
    if (str == "ASYNC") return strategy_type::ASYNC;
    if (str == "BLOCKING") return strategy_type::BLOCKING;
    return strategy_type::BLOCKING; // default
}

std::string pack_strategy_factory::strategy_type_to_string(strategy_type type) {
    switch (type) {
        case strategy_type::BLOCKING: return "BLOCKING";
        case strategy_type::PARALLEL: return "PARALLEL";
        case strategy_type::ASYNC: return "ASYNC";
        default: return "BLOCKING";
    }
}
