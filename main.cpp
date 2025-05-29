#include <iostream>
#include <string>
#include "pack_planner_app.h"
#include "benchmark.h"
#include "timer.h"

using namespace pplanner;
using namespace utils;

// Main function with integrated timing and benchmark support
int main(int argc, char* argv[]) {
    timer total_timer;
    total_timer.start();

    try {
        // Check for benchmark mode
        if (argc > 1 && std::string(argv[1]) == "--benchmark") {
            benchmark_runner::warmup();
            benchmark_runner::run_performance_test();
            total_timer.stop();
            std::cout << "\n";
            total_timer.print_time("Total benchmark execution");
            return 0;
        }

        // Check for quiet mode (disable timing)
        bool enable_timing = true;
        int file_arg_index = 1;

        if (argc > 1 && std::string(argv[1]) == "--quiet") {
            enable_timing = false;
            file_arg_index = 2;
        }

        // Regular execution with timing
        if (argc > file_arg_index) {
            // File input mode
            int result = pack_planner_app::run_from_file(argv[file_arg_index], enable_timing);
            total_timer.stop();
            if (enable_timing) {
                std::cout << "\n";
                total_timer.print_time("Total execution");
            }
            return result;
        } else if (argc == 1 || (argc == 2 && !enable_timing)) {
            // Console input mode
            int result = pack_planner_app::run_from_console(enable_timing);
            total_timer.stop();
            if (enable_timing) {
                std::cout << "\n";
                total_timer.print_time("Total execution");
            }
            return result;
        } else {
            std::cout << "Usage: " << argv[0] << " [--benchmark|--quiet] [input_file]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --benchmark     Run performance benchmark tests" << std::endl;
            std::cout << "  --quiet         Disable timing output" << std::endl;
            std::cout << "  input_file      Read from specified file" << std::endl;
            std::cout << "  (no args)       Read from console (end with empty line)" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        total_timer.stop();
        std::cerr << "Fatal error: " << e.what() << std::endl;
        std::cerr << "Execution time before error: " << total_timer.get_milliseconds() << " ms" << std::endl;
        return 1;
    }
}
