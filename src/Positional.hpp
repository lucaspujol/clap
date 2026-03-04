#pragma once

#include <string>
#include <optional>

#include "IArgument.hpp"
#include "TypeNames.hpp"
#include "ClapExceptions.hpp"

namespace clap {
    template<typename T>
    class Positional : public IArgument {
        public:
            Positional(std::string names, std::string description)
            : IArgument(std::move(names), std::move(description)) {}

            void parse(std::string_view value) override {
                std::istringstream iss{std::string(value)};
                T val;
                if (!(iss >> val))
                    throw clap::ClapException("Failed to parse value");
                _value = std::move(val);
            }

            std::string_view type_name() const override {
                return clap::TypeName<T>::value;
            }

            bool is_set() const noexcept override { return _value.has_value(); }
            bool takes_value() const noexcept override { return true; }

            const T &get() const {
                if (_value.has_value())
                    return _value.value();
                throw clap::MissingValue(std::string(names()));
            }

        private:
            std::optional<T> _value;
    };
}
