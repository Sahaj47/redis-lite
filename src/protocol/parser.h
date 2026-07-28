#pragma once
#include <string>
#include <vector>

// A structured container for user commands
struct Command {
    std::string name;              // e.g., "SET", "GET", "DEL"
    std::vector<std::string> args; // e.g., ["username", "Sahaj"]
};

class Parser {
public:
    // Takes a raw string and breaks it into a Command struct
    static Command parse(const std::string& raw_input);
};