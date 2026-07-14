#pragma once

#include <string>
#include <string_view>
#include <sstream>

#include "ClapExceptions.hpp"

namespace clap {
    template<typename T>
    struct ParseValue {
        static T parse(std::string_view str) {
            std::istringstream iss{std::string(str)};
            T val;
            if (!(iss >> val) || !iss.eof())
                throw clap::ParseError("Failed to parse value");
            return val;
        }
    };

    template<>
    struct ParseValue<std::string> {
        static std::string parse(std::string_view str) {
            return std::string(str);
        }
    };
}