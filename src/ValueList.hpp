#pragma once

#include "Argument.hpp"
#include "ParseValue.hpp"
#include "ClapExceptions.hpp"
#include "TypeNames.hpp"

#include <vector>

namespace clap {
    /// Collects multiple parsed values of T into a list. Backs both a repeated
    /// named option (-t a -t b) and a variadic positional (prog a b c); the two
    /// differ only in how App routes tokens to them, not in how they store.
    template<typename T>
    class ValueList : public Argument {
    public:
        ValueList(std::string names, std::string description)
        : Argument(std::move(names), std::move(description)) {}

        void parse(std::string_view value, bool discard) override {
            auto v = clap::parse_checked<T>(value, names(), type_name());
            if (!discard) _values.push_back(std::move(v));
        }

        std::string_view type_name() const override {
            return clap::TypeName<T>::value;
        }

        bool is_set() const noexcept override { return !_values.empty(); }
        bool takes_value() const noexcept override { return true; }
        bool is_multi() const noexcept override { return true; }

        /// Require at least one value. Parsing fails if none is given.
        ValueList<T>& required() {
            set_required();
            return *this;
        }

        /// All collected values. Empty when nothing was given and not required;
        /// throws MissingValue only if this list is required but stayed empty.
        const std::vector<T>& get() const {
            if (_values.empty() && is_required())
                throw clap::MissingValue(std::string(names()));
            return _values;
        }

    private:
        std::vector<T> _values;
    };
}
