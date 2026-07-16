#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <istream>
#include <ostream>

#include "ClapExceptions.hpp"

namespace clap {
    template<typename T>
    concept StreamExtractable = requires(std::istream& is, T& v) { is >> v; };

    template<typename T>
    concept StreamInsertable = requires(std::ostream& os, const T& v) { os << v; };

    template<typename T>
    struct ParseValue;

    /// Default parser for any type readable with operator>>.
    template<StreamExtractable T>
    struct ParseValue<T> {
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

    /// either ParseValue<T> is specialized, or T is stream-extractable. 
    template<typename T>
    concept Parseable = requires(std::string_view s) { ParseValue<T>::parse(s); };

    /// The full contract for a clap value type: parseable from a string, and
    /// printable via operator<< so its default value can appear in help output.
    template<typename T>
    concept OptionValue = Parseable<T> && StreamInsertable<T>;

    /// Parses value into T, turning any failure into an InvalidValue error.
    template<typename T>
    T parse_checked(std::string_view value, std::string_view name, std::string_view type) {
        try {
            return ParseValue<T>::parse(value);
        } catch (const clap::ParseError&) {
            throw clap::InvalidValue(std::string(value), std::string(name), std::string(type));
        }
    }
}