#pragma once

#include <string>
#include <optional>
#include <sstream>

#include "Argument.hpp"
#include "TypeNames.hpp"
#include "ClapExceptions.hpp"
#include "ParseValue.hpp"

namespace clap {
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

            Positional<T>& required() {
                if (_default_value.has_value())
                    throw clap::ConfigError("cannot combine required() with default_value()");
                set_required();
                return *this;
            }

            Positional<T>& default_value(T val) {
                if (is_required())
                    throw clap::ConfigError("cannot combine default_value() with required()");
                _default_value = std::move(val);
                return *this;
            }

            std::string default_str() const override {
                if (!_default_value.has_value()) return "";
                std::ostringstream oss;
                oss << _default_value.value();
                return oss.str();
            }

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
