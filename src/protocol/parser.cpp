#include "parser.h"
#include <sstream>
#include <cctype>

Command Parser::parse(const std::string& raw_input) {
    Command cmd;
    std::istringstream stream(raw_input);
    std::string token;

    // 1. Extract the first word as the command name
    if (stream >> token) {
        cmd.name = token;
        // Convert the command to uppercase so "set", "Set", and "SET" all work
        for (char& c : cmd.name) {
            c = std::toupper(c);
        }
    }

    // 2. Extract all remaining words as arguments
    while (stream >> token) {
        cmd.args.push_back(token);
    }

    return cmd;
}