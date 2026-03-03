#pragma once

#include <string>

namespace clap {
    template<typename T>
    struct TypeName {
        static constexpr std::string_view value = "unknown";
    };

    template<> struct TypeName<int>         { static constexpr std::string_view value = "int"; };
    template<> struct TypeName<float>       { static constexpr std::string_view value = "float"; };
    template<> struct TypeName<double>      { static constexpr std::string_view value = "double"; };
    template<> struct TypeName<bool>        { static constexpr std::string_view value = "bool"; };
    template<> struct TypeName<std::string> { static constexpr std::string_view value = "string"; };
}
