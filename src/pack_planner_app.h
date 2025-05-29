#ifndef PACK_PLANNER_APP
#define PACK_PLANNER_APP

#include <iostream>
#include <fstream>
#include <string>
#include "pack_planner.h"
#include "input_parser.h"
#include "timer.h"

// Main application class
class pack_planner_app {
public:
    static int run_from_file(const std::string& filename, bool enable_timing = true) {
        utils::timer file_timer;
        if (enable_timing) {
            file_timer.start();
            std::cout << "Reading from file: " << filename << std::endl;
        }

        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return 1;
        }

        int result = run_from_stream(file, enable_timing);

        if (enable_timing) {
            file_timer.stop();
            file_timer.print_time("File I/O");
        }

        return result;
    }

    static int run_from_console(bool enable_timing = true) {
        std::cout << "Enter pack planner input (empty line to finish):" << std::endl;
        return run_from_stream(std::cin, enable_timing);
    }

    static int run_from_stream(std::istream& input, bool enable_timing = true) {
        utils::timer parse_timer;
        if (enable_timing) parse_timer.start();

        try {
            std::string line;

            // Read header
            if (!std::getline(input, line) || line.empty()) {
                std::cerr << "Error: Missing header line" << std::endl;
                return 1;
            }

            pplanner::sort_order order;
            int max_items;
            double max_weight;

            if (!pplanner::input_parser::parse_header(line, order, max_items, max_weight)) {
                std::cerr << "Error: Invalid header format" << std::endl;
                return 1;
            }

            pplanner::pack_planner planner(max_items, max_weight, order, enable_timing);

            // Read items
            int item_count = 0;
            while (std::getline(input, line) && !line.empty()) {
                pplanner::item parsed_item(0, 0, 1, 0.0); // Default constructor values
                if (pplanner::input_parser::parse_item_line(line, parsed_item)) {
                    planner.add_item(parsed_item);
                    item_count++;
                } else {
                    std::cerr << "Warning: Skipping invalid item line: " << line << std::endl;
                }
            }

            if (enable_timing) {
                parse_timer.stop();
                parse_timer.print_time("Input parsing");
                std::cout << "Items loaded: " << item_count << std::endl;
            }

            planner.plan_packs();
            planner.print_result();
            return 0;

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }
};

#endif // PACK_PLANNER_APP
