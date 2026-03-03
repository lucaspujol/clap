#pragma once

#include "IArgument.hpp"

#include <string>

namespace clap {
    class Flag : public IArgument {
        public:
            Flag(std::string names, std::string description)
            : IArgument(std::move(names), std::move(description)) {}

            operator bool() const noexcept { return _value; }

            void parse(std::string_view) override { _value = true; }
            std::string_view type_name() const override { return ""; }

            bool is_set() const noexcept override { return _value; }
            bool takes_value() const noexcept override { return false; }

        private:
            bool _value = false;
    };
}