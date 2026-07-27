#pragma once

#include <istream>
#include <ostream>
#include <string_view>

#include "TypeNames.hpp"

namespace clap {
    /// Satisfied when T can be read from an std::istream with operator>>.
    template<typename T>
    concept StreamExtractable = requires(std::istream& is, T& v) { is >> v; };

    /// Satisfied when T can be written to an std::ostream with operator<<.
    template<typename T>
    concept StreamInsertable = requires(std::ostream& os, const T& v) { os << v; };

    /// Satisfied when T has a TypeName, so it has a label for help output.
    template<typename T>
    concept Named = requires { TypeName<T>::value; };

    /// Turns a string into a T. The customization point for value parsing:
    /// specialize this for a type that operator>> cannot handle. See
    /// examples/custom_type.
    /// Declared here so Parseable below can name it;
    /// the default parser and the built-in specializations live in
    /// ParseValue.hpp.
    template<typename T>
    struct ParseValue;

    /// Satisfied when T has a usable ParseValue: either ParseValue<T> is
    /// specialized, or T is stream-extractable via the default parser.
    template<typename T>
    concept Parseable = requires(std::string_view s) { ParseValue<T>::parse(s); };

    /// The full contract for a clap value type: parseable from a string, and
    /// printable via operator<< so its default value can appear in help output.
    template<typename T>
    concept OptionValue = Parseable<T> && StreamInsertable<T> && Named<T>;
}
