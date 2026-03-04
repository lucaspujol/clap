#pragma once

#include <string>
#include <optional>

#include "IArgument.hpp"
#include "TypeNames.hpp"
#include "ClapExceptions.hpp"
#include "ParseValue.hpp"

namespace clap {
    template<typename T>
    class Option : public IArgument {
        public:
            Option(std::string names, std::string description)
            : IArgument(std::move(names), std::move(description)) {}

            // TODO: better parsing
            void parse(std::string_view value) override {
                _value = clap::ParseValue<T>::parse(value);
            }

            std::string_view type_name() const override {
                return clap::TypeName<T>::value;
            }
            bool is_set() const noexcept override { return _value.has_value(); }

            Option<T>& required() {
                if (_default_value.has_value())
                    throw clap::ConfigError("cannot combine required() with default_value()");
                set_required();
                return *this;
            }

            bool takes_value() const noexcept override { return true; }

            Option<T>& default_value(T val) {
                if (is_required())
                    throw clap::ConfigError("cannot combine default_value() with required()");
                _default_value = std::move(val);
                return *this;
            }

            const T& get() const {
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
