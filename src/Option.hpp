#pragma once

#include <string>
#include <optional>
#include <sstream>

#include "Argument.hpp"
#include "TypeNames.hpp"
#include "ClapExceptions.hpp"
#include "ParseValue.hpp"

namespace clap {
    /// A named argument that takes one typed value, e.g. -c 10 or --count=10.
    template<typename T>
    class Option : public Argument {
        public:
            Option(std::string names, std::string description)
            : Argument(std::move(names), std::move(description)) {}

            void parse(std::string_view value) override {
                _value = clap::parse_checked<T>(value, names(), type_name());
            }

            std::string_view type_name() const override {
                return clap::TypeName<T>::value;
            }
            bool is_set() const noexcept override { return _value.has_value(); }

            /// Mark as required. Parsing fails if absent. Excludes default_value().
            Option<T>& required() {
                if (_default_value.has_value())
                    throw clap::ConfigError("cannot combine required() with default_value()");
                set_required();
                return *this;
            }

            /// option always takes a value, so this is true. (Flag overrides to false.)
            bool takes_value() const noexcept override { return true; }

            std::string default_str() const override {
                if (!_default_value.has_value()) return "";
                std::ostringstream oss;
                oss << _default_value.value();
                return oss.str();
            }

            /// Set a fallback used when the option is absent. Excludes required().
            Option<T>& default_value(T val) {
                if (is_required())
                    throw clap::ConfigError("cannot combine default_value() with required()");
                _default_value = std::move(val);
                return *this;
            }

            /// The parsed value, else the default. Throws MissingValue if neither.
            const T& get() const {
                if (_value.has_value())
                    return _value.value();
                if (_default_value.has_value())
                    return _default_value.value();
                throw clap::MissingValue(std::string(names()));
            }

            /// The parsed value, else the default, else a fallback. Never throws.
            T get_or(T fallback) const {
                if (_value.has_value())
                    return _value.value();
                if (_default_value.has_value())
                    return _default_value.value();
                return fallback;
            }

        private:
            std::optional<T> _value;
            std::optional<T> _default_value;
    };
}
