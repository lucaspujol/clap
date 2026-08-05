#pragma once

#include <charconv>
#include <cmath>
#include <filesystem>
#include <string>
#include <string_view>
#include <sstream>
#include <system_error>
#include <type_traits>

#include "ClapExceptions.hpp"
#include "Concepts.hpp"

namespace clap {
    /// Default parser for any type readable with operator>>.
    template<StreamExtractable T>
    struct ParseValue<T> {
        static T parse(std::string_view str) {
            T val;
            if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>) {
                auto* first = str.data();
                auto* last = str.data() + str.size();
                auto [ptr, ec] = std::from_chars(first, last, val);
                if (ec == std::errc::invalid_argument)
                    throw clap::ParseError("");
                if (ec == std::errc::result_out_of_range)
                    throw clap::ParseError("out of range");
                if (ptr != last)
                    throw clap::ParseError("");
                if constexpr (std::is_floating_point_v<T>)
                    if (!std::isfinite(val))
                        throw clap::ParseError("must be finite");
                return val;
            } else {
                std::istringstream iss{std::string(str)};
                if (!(iss >> val) || !iss.eof())
                    throw clap::ParseError("");
                return val;
            }
        }
    };

    /// Parser for std::string: takes the token verbatim, no stream parsing
    /// (so values may contain spaces and never fail to parse).
    template<>
    struct ParseValue<std::string> {
        static std::string parse(std::string_view str) { return std::string(str); }
    };

    template<>
    struct ParseValue<std::filesystem::path> {
        static std::filesystem::path parse(std::string_view str) { return std::filesystem::path(str); }
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

    /// Parser for char: the token is a single character, not a number, so
    /// `-c A` reads the letter A. signed/unsigned char keep the numeric path.
    template<>
    struct ParseValue<char> {
        static char parse(std::string_view str) {
            if (str.size() != 1)
                throw ParseError("expected a single character");
            return str[0];
        }
    };

    /// Parses value into T, turning any failure into an InvalidValue error.
    template<typename T>
    T parse_checked(std::string_view value, std::string_view name, std::string_view type) {
        try {
            return ParseValue<T>::parse(value);
        } catch (const clap::ParseError &e) {
            throw clap::InvalidValue(std::string(value), std::string(name), std::string(type), e.detail());
        } catch (const std::exception &e) {
            throw clap::InvalidValue(std::string(value), std::string(name), std::string(type), e.what());
        }
    }
}
