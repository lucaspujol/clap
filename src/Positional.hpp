#pragma once

#include <string>
#include <optional>
#include <sstream>

#include "Argument.hpp"
#include "TypeNames.hpp"
#include "ClapExceptions.hpp"
#include "ParseValue.hpp"

namespace clap {
    /// An order-based argument with no dash, e.g. an input file.
    /// Required unless given a default_value().
    template<typename T>
    class Positional : public Argument {
        public:
            Positional(std::string names, std::string description)
            : Argument(std::move(names), std::move(description)) {}

            void parse(std::string_view value) override {
                _value = clap::parse_checked<T>(value, names(), type_name());
            }

            std::string_view type_name() const override {
                return clap::TypeName<T>::value;
            }

            bool is_set() const noexcept override { return _value.has_value(); }
            bool takes_value() const noexcept override { return true; }

            bool is_required() const noexcept override { return !_default_value.has_value(); }

            /// Set a fallback value, making the positional optional.
            Positional<T>& default_value(T val) {
                _default_value = std::move(val);
                return *this;
            }

            std::string default_str() const override {
                if (!_default_value.has_value()) return "";
                std::ostringstream oss;
                oss << _default_value.value();
                return oss.str();
            }

            /// The parsed value, else the default. Throws MissingValue if neither.
            const T &get() const {
                if (_value.has_value())
                    return _value.value();
                if (_default_value.has_value())
                    return _default_value.value();
                throw clap::MissingValue(std::string(names()));
            }

        private:
            std::optional<T> _value;
            std::optional<T> _default_value;
    };
}
