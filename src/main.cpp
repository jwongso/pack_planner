#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <CLI/CLI.hpp>

#include "item.h"
#include "pack_planner.h"
#include "benchmark.h"

strategy_type parse_strategy_type(const std::string& str) {
    if (str == "BLOCKING_FIRST_FIT") return strategy_type::BLOCKING_FIRST_FIT;
    if (str == "PARALLEL_FIRST_FIT") return strategy_type::PARALLEL_FIRST_FIT;
    if (str == "LOCKFREE_FIRST_FIT") return strategy_type::LOCKFREE_FIRST_FIT;
    return strategy_type::BLOCKING_FIRST_FIT;
}

bool load_items_from_file(const std::string& filename, std::vector<item>& items) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    items.clear();
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        int id, length, quantity;
        double weight;
        char comma;

        if (iss >> id >> comma >> length >> comma >> quantity >> comma >> weight) {
            items.emplace_back(id, length, quantity, weight);
        }
    }

    file.close();
    return !items.empty();
}

int main(int argc, char* argv[]) {
    CLI::App app{"Pack Planner"};

    std::string input_file;
    std::string output_file = "output.txt";
    std::string strategy_str = "BLOCKING_FIRST_FIT";
    std::string sort_order_str = "NATURAL";
    int max_items_per_pack = 100;
    double max_weight_per_pack = 200.0;
    int thread_count = 4;
    bool run_benchmark = false;
    bool run_sort_benchmark = false;
    bool run_thread_benchmark = false;
    std::vector<unsigned int> thread_counts = {1, 4, 8, 12, 16, 24};

    app.add_option("-i,--input", input_file, "Input CSV file path");
    app.add_option("-o,--output", output_file, "Output file path");
    app.add_option("-s,--strategy", strategy_str, "Packing strategy");
    app.add_option("--sort", sort_order_str, "Sort order");
    app.add_option("-m,--max-items", max_items_per_pack, "Maximum items per pack");
    app.add_option("-w,--max-weight", max_weight_per_pack, "Maximum weight per pack");
    app.add_option("-t,--threads", thread_count, "Number of threads");
    app.add_flag("-b,--benchmark", run_benchmark, "Run performance benchmarks");
    app.add_flag("--benchmark-sort", run_sort_benchmark, "Run sorting algorithm benchmarks");
    app.add_flag("--benchmark-threads", run_thread_benchmark, "Run thread scaling benchmarks");
    app.add_option("--thread-counts", thread_counts, "Thread counts for benchmark");

    CLI11_PARSE(app, argc, argv);

    if (run_sort_benchmark) {
        benchmark::benchmark_sorts();
        return 0;
    }

    if (run_thread_benchmark) {
        benchmark bench;
        bench.run_benchmark_with_threads(thread_counts);
        return 0;
    }

    if (run_benchmark) {
        benchmark bench;
        bench.run_benchmarks();
        return 0;
    }

    if (input_file.empty()) {
        return 1;
    }

    std::vector<item> items;
    if (!load_items_from_file(input_file, items)) {
        return 1;
    }

    pack_planner_config config;
    config.type = parse_strategy_type(strategy_str);
    config.order = parse_sort_order(sort_order_str);
    config.max_items_per_pack = max_items_per_pack;
    config.max_weight_per_pack = max_weight_per_pack;
    config.thread_count = thread_count;

    pack_planner planner;
    auto result = planner.plan_packs(config, items);

    std::ofstream file(output_file);
    if (file.is_open()) {
        planner.output_results(result.packs, file);
        file.close();
    }

    return 0;
}
