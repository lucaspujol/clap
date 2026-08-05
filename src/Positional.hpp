#pragma once

#include <locale>
#include <string>
#include <optional>
#include <sstream>

#include "ClapExceptions.hpp"
#include "TypedArgument.hpp"

namespace clap {
    /// An order-based argument with no dash, e.g. an input file.
    /// Required unless given a default_value().
    /// CRTP: inherits from himself. this is used to return the derived type from
    /// methods like required() and default_value().
    template<typename T>
    class Positional : public TypedArgument<T, Positional<T>> {
        public:
            Positional(std::string names, std::string description)
            : TypedArgument<T, Positional<T>>(std::move(names), std::move(description)) {}

            using Argument::parse;
            void parse(std::string_view value, bool) override {
                _value = this->parse_value(value);
            }

            bool is_set() const noexcept override { return _value.has_value(); }

            bool is_required() const noexcept override { return !_default_value.has_value(); }

            /// Set a fallback value, making the positional optional.
            Positional<T>& default_value(T val) {
                _default_value = std::move(val);
                return *this;
            }

            std::string default_str() const override {
                if (!_default_value.has_value()) return "";
                std::ostringstream oss;
                oss.imbue(std::locale::classic());
                oss << std::boolalpha << _default_value.value();
                return oss.str();
            }

            /// The parsed value, else the default. Throws MissingValue if neither.
            const T&  get() const {
                if (_value.has_value())
                    return _value.value();
                if (_default_value.has_value())
                    return _default_value.value();
                throw clap::MissingValue(std::string(this->names()));
            }

            void reset() noexcept override { _value.reset(); }

        private:
            std::optional<T> _value;
            std::optional<T> _default_value;
    };
}
