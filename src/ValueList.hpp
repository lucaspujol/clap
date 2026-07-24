#pragma once

#include "ClapExceptions.hpp"
#include "TypedArgument.hpp"

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
        bool takes_value() const noexcept override { return true; }
        bool is_multi() const noexcept override { return true; }

        /// Require at least one value. Parsing fails if none is given.
        ValueList<T>& required() {
            this->set_required();
            return *this;
        }

        /// All collected values. Empty when nothing was given and not required;
        /// throws MissingValue only if this list is required but stayed empty.
        const std::vector<T>& get() const {
            if (_values.empty() && this->is_required())
                throw clap::MissingValue(std::string(this->names()));
            return _values;
        }

    private:
        std::vector<T> _values;
    };
}
