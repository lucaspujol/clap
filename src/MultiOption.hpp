#pragma once

#include "Argument.hpp"
#include "ParseValue.hpp"
#include "ClapExceptions.hpp"
#include "TypeNames.hpp"

#include <vector>

namespace clap {
    template<typename T>
    class MultiOption : public Argument {
    public:
        MultiOption(std::string names, std::string description)
        : Argument(std::move(names), std::move(description)) {}

        void parse(std::string_view value) override {
            _values.push_back(clap::parse_checked<T>(value, names(), type_name()));
        }

        std::string_view type_name() const override {
            return clap::TypeName<T>::value;
        }

        bool is_set() const noexcept override { return !_values.empty(); }
        bool takes_value() const noexcept override { return true; }
        bool is_multi() const noexcept override { return true; }

        MultiOption<T>& required() {
            set_required();
            return *this;
        }

        const std::vector<T>& get() const {
            if (_values.empty())
                throw clap::MissingValue(std::string(names()));
            return _values;
        }

    private:
        std::vector<T> _values;
    };
}
