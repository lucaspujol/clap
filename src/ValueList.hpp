#pragma once

#include "ClapExceptions.hpp"
#include "TypedArgument.hpp"

#include <sstream>
#include <vector>

namespace clap {
    /// Collects multiple parsed values of T into a list. Backs both a repeated
    /// named option (-t a -t b) and a variadic positional (prog a b c); the two
    /// differ only in how App routes tokens to them, not in how they store.
    /// CRTP: inherits from himself. this is used to return the derived type from
    /// methods like required() and default_value().
    template<typename T>
    class ValueList : public TypedArgument<T, ValueList<T>> {
    public:
        ValueList(std::string names, std::string description)
        : TypedArgument<T, ValueList<T>>(std::move(names), std::move(description)) {}

        void parse(std::string_view value, bool discard) override {
            auto v = this->parse_value(value);
            if (!discard) _values.push_back(std::move(v));
        }

        bool is_set() const noexcept override { return !_values.empty(); }
        bool is_multi() const noexcept override { return true; }

        /// Require at least one value. Parsing fails if none is given.
        /// Excludes default_value().
        ValueList<T>& required() {
            if (!_default_values.empty())
                throw clap::ConfigError("cannot combine required() with default_value()");
            this->set_required();
            return *this;
        }

        /// Set a fallback list used when no value is given. Excludes required().
        /// An empty list is the same as no default: get() returns empty either way.
        ValueList<T>& default_value(std::vector<T> values) {
            if (this->is_required())
                throw clap::ConfigError("cannot combine default_value() with required()");
            _default_values = std::move(values);
            return *this;
        }

        std::string default_str() const override {
            if (_default_values.empty()) return "";
            std::ostringstream oss;
            for (size_t i = 0; i < _default_values.size(); ++i) {
                if (i) oss << ',';
                oss << _default_values[i];
            }
            return oss.str();
        }

        /// All collected values, else the default list. Empty when neither was
        /// given and not required; throws MissingValue only if this list is
        /// required but stayed empty.
        const std::vector<T>& get() const {
            if (_values.empty()) {
                if (this->is_required())
                    throw clap::MissingValue(std::string(this->names()));
                return _default_values;
            }
            return _values;
        }

    private:
        std::vector<T> _values;
        std::vector<T> _default_values;
    };
}
