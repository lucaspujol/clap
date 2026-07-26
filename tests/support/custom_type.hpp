#pragma once

// A user-defined value type, taught to clap the way examples/custom_type does:
//   - TypeName for the help label
//   - ParseValue for the conversion
//   - operator<< for rendering a default.
//
// It is the only type that reaches the istringstream-free custom path, so it
// belongs in the type matrix and in the coverage probe.
// see src/ParseValue.hpp

#include "clap_header.hpp"

#include <ostream>
#include <string_view>

enum class Mode { Fast, Safe, Debug };

inline std::ostream& operator<<(std::ostream& os, Mode m) {
    switch (m) {
        case Mode::Fast:  return os << "fast";
        case Mode::Safe:  return os << "safe";
        case Mode::Debug: return os << "debug";
    }
    return os << "?";
}

namespace clap {
    template<> struct TypeName<Mode> {
        static constexpr std::string_view value = "mode";
    };

    template<> struct ParseValue<Mode> {
        static Mode parse(std::string_view s) {
            if (s == "fast")  return Mode::Fast;
            if (s == "safe")  return Mode::Safe;
            if (s == "debug") return Mode::Debug;
            throw clap::ParseError("expected one of: fast, safe, debug");
        }
    };
}
