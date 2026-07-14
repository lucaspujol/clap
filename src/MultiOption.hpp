#pragma once

#include "IArgument.hpp"
#include "ParseValue.hpp"
#include "ClapExceptions.hpp"
#include "TypeNames.hpp"

#include <vector>
#include <sstream>

namespace clap {
    template<typename T>
    class MultiOption : public IArgument {
    public:
        MultiOption(std::string names, std::string description)
        : IArgument(std::move(names), std::move(description)) {}

        void parse(std::string_view value) override {
            _values.push_back(clap::parse_checked<T>(value, names(), type_name()));
        }

        std::string_view type_name() const override {
            return clap::TypeName<T>::value;
        }

        bool is_set() const noexcept override { return !_values.empty(); }
        bool takes_value() const noexcept override { return true; }

        MultiOption<T>& required() {
            set_required();
            return *this;
        }

        const std::vector<T>& get() const {
            if (_values.empty())
                throw clap::MissingValue(std::string(names()));
            return _values;
        }

        std::string prefix() const override {
            std::ostringstream oss;
            oss << "  " << names() << " <" << type_name() << ">...";
            return oss.str();
        }

        std::string usage_token() const override {
            std::string core = std::string(primary_name()) + " <" + std::string(type_name()) + ">";
            return is_required() ? core + "..." : "[" + core + "]...";
        }

    private:
        std::vector<T> _values;
    };
}
