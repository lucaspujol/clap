#pragma once

#include "Argument.hpp"

#include <string>

namespace clap {
    /// A boolean switch like -v or --force. Convert to bool to read it.
    class Flag : public Argument {
        public:
            Flag(std::string names, std::string description)
            : Argument(std::move(names), std::move(description)) {}

            operator bool() const noexcept { return _value; }

            void parse(std::string_view) override { _value = true; }
            std::string_view type_name() const override { return ""; }

            bool is_set() const noexcept override { return _value; }
            bool takes_value() const noexcept override { return false; }

        private:
            bool _value = false;
    };
}