#pragma once

#include "Argument.hpp"

#include <string>

namespace clap {
    /// A boolean switch like -v or --force. Convert to bool to read it.
    /// You can also call .count() to see how many times it was repeated.
    /// example: -vvv sets count() to 3 (if you want a level of verboseness).
    class Flag : public Argument {
        public:
            Flag(std::string names, std::string description)
            : Argument(std::move(names), std::move(description)) {}

            operator bool() const noexcept { return _value; }

            void parse(std::string_view, bool discard) noexcept override {
                if (!discard) {
                    _value = true;
                    _count++;
                }
            }

            std::string_view type_name() const override { return ""; }

            bool is_set() const noexcept override { return _value; }
            bool takes_value() const noexcept override { return false; }

            /// How many times the flag was set. 0 when absent, 3 for -vvv.
            /// Discarded occurrences (-/v) don't count.
            int count() const noexcept { return _count; }

        private:
            bool _value = false;
            int _count = 0;
    };
}
