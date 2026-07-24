#pragma once

#include <string>
#include <optional>
#include <sstream>

#include "TypedArgument.hpp"
#include "ClapExceptions.hpp"

namespace clap {
    /// A named argument that takes one typed value, e.g. -c 10 or --count=10.
    /// CRTP: inherits from himself. this is used to return the derived type from 
    /// methods like required() and default_value().
    template<typename T>
    class Option : public TypedArgument<T, Option<T>> {
        public:
            Option(std::string names, std::string description)
            : TypedArgument<T, Option<T>>(std::move(names), std::move(description)) {}

            void parse(std::string_view value, bool discard) override {
                auto v = this->parse_value(value);
                if (!discard) _value = std::move(v);
            }
            
            bool is_set() const noexcept override { return _value.has_value(); }

            /// Mark as required. Parsing fails if absent. Excludes default_value().
            Option<T>& required() {
                if (_default_value.has_value())
                    throw clap::ConfigError("cannot combine required() with default_value()");
                this->set_required();
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
                if (this->is_required())
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
                throw clap::MissingValue(std::string(this->names()));
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
