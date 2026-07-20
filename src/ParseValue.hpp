#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <istream>
#include <ostream>

#include "ClapExceptions.hpp"

namespace clap {
    /// Satisfied when T can be read from an std::istream with operator>>.
    template<typename T>
    concept StreamExtractable = requires(std::istream& is, T& v) { is >> v; };

    /// Satisfied when T can be written to an std::ostream with operator<<.
    template<typename T>
    concept StreamInsertable = requires(std::ostream& os, const T& v) { os << v; };

    /// Turns a string into a T. The customization point for value parsing:
    /// specialize this for a type that operator>> cannot handle. See
    /// examples/custom_type.
    template<typename T>
    struct ParseValue;

    /// Default parser for any type readable with operator>>.
    template<StreamExtractable T>
    struct ParseValue<T> {
        static T parse(std::string_view str) {
            std::istringstream iss{std::string(str)};
            T val;
            if (!(iss >> val) || !iss.eof())
                throw clap::ParseError("");
            return val;
        }
    };

    /// Parser for std::string: takes the token verbatim, no stream parsing
    /// (so values may contain spaces and never fail to parse).
    template<>
    struct ParseValue<std::string> {
        static std::string parse(std::string_view str) {
            return std::string(str);
        }
    };

    template<>
    struct ParseValue<bool> {
        static bool parse(std::string_view str) {
            if (str == "1" || str == "true" || str == "yes" || str == "on")
                return true;
            if (str == "0" || str == "false" || str == "no" || str == "off")
                return false;
            throw ParseError("valid values: 1, true, yes, on, 0, false, no, off");
        }
    };

    /// Satisfied when T has a usable ParseValue: either ParseValue<T> is
    /// specialized, or T is stream-extractable via the default parser.
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
        } catch (const clap::ParseError &e) {
            throw clap::InvalidValue(std::string(value), std::string(name), std::string(type), e.detail());
        }
    }
}