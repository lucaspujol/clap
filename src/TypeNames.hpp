#pragma once

#include <filesystem>
#include <string>

namespace clap {
    /// Label shown for a value type in help output, e.g. "<int>".
    /// Specialize for a custom type to give it a name.
    template<typename T> struct TypeName;

    // macro to define all accepted types
    #define CLAP_TYPENAME(T, S) \
        template<> struct TypeName<T> { static constexpr std::string_view value = S; };

    // Fundamental types only. 
    // uint8_t is unsigned char, size_t is one of the unsigned types,
    // so naming both collides.
    CLAP_TYPENAME(char,                  "char")
    CLAP_TYPENAME(signed char,           "int8")
    CLAP_TYPENAME(unsigned char,         "uint8")
    CLAP_TYPENAME(short,                 "short")
    CLAP_TYPENAME(unsigned short,        "ushort")
    CLAP_TYPENAME(int,                   "int")
    CLAP_TYPENAME(unsigned,              "uint")
    CLAP_TYPENAME(long,                  "long")
    CLAP_TYPENAME(unsigned long,         "ulong")
    CLAP_TYPENAME(long long,             "llong")
    CLAP_TYPENAME(unsigned long long,    "ullong")
    CLAP_TYPENAME(float,                 "float")
    CLAP_TYPENAME(double,                "double")
    CLAP_TYPENAME(bool,                  "bool")
    CLAP_TYPENAME(std::string,           "string")
    CLAP_TYPENAME(std::filesystem::path, "path")

}
