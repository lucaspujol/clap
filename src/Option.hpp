#pragma once

#include <string>
#include <optional>

#include "IArgument.hpp"
#include "TypeNames.hpp"

namespace clap {
    template<typename T>
    class Option : public IArgument {
        public:
            Option(std::string names, std::string description)
            : IArgument(std::move(names), std::move(description)) {}

            // TODO: better parsing
            void parse(std::string_view value) override {
                std::istringstream iss{std::string(value)};
                T val;
                if (!(iss >> val))
                    throw std::runtime_error("Failed to parse value");
                _value = std::move(val);
            }

            std::string_view type_name() const override {
                return clap::TypeName<T>::value;
            }
            bool is_set() const noexcept override { return _value.has_value(); }

            Option<T>& required() noexcept {
                set_required();
                return *this;
            }

            bool takes_value() const noexcept override { return true; }

            Option<T>& default_value(T val) noexcept {
                _value = std::move(val);
                return *this;
            }

            const T& get() const {
                if (_value.has_value())
                    return _value.value();
                throw std::runtime_error("No value set");
            }

        private:
            std::optional<T> _value;
    };
}
