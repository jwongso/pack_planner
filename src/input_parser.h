#ifndef INPUT_PARSER_H
#define INPUT_PARSER_H

#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include "item.h"
#include "sorter.h"

namespace pplanner {

// Input parsing utilities
class input_parser {
public:
    static sort_order parse_sort_order(const std::string& order_str) {
        if (order_str == "NATURAL") return sort_order::NATURAL;
        if (order_str == "SHORT_TO_LONG") return sort_order::SHORT_TO_LONG;
        if (order_str == "LONG_TO_SHORT") return sort_order::LONG_TO_SHORT;
        throw std::invalid_argument("Invalid sort order: " + order_str);
    }

    static std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    static bool parse_header(const std::string& line, sort_order& order,
                           int& max_items, double& max_weight) {
        try {
            auto tokens = split(line, ',');
            if (tokens.size() != 3) return false;

            order = parse_sort_order(tokens[0]);
            max_items = std::stoi(tokens[1]);
            max_weight = std::stod(tokens[2]);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    static bool parse_item_line(const std::string& line, item& parsed_item) {
        try {
            auto tokens = split(line, ',');
            if (tokens.size() != 4) return false;

            int id = std::stoi(tokens[0]);
            int length = std::stoi(tokens[1]);
            int quantity = std::stoi(tokens[2]);
            double weight = std::stod(tokens[3]);

            parsed_item = item(id, length, quantity, weight);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
};

}

#endif // INPUT_PARSER_H
