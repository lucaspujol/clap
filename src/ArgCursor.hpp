#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace clap {
    /// Walks the argument list left to right, one token at a time
    /// (skips args[0], the program name). Does not own the list.
    class ArgCursor {
        public:
            explicit ArgCursor(const std::vector<std::string>& args) noexcept
                : _args(args), _pos(1) {}

            bool has_next() const noexcept { return _pos < _args.size(); }

            /// Index of the token next() would return. Used to tag an error
            /// with the argv slot that caused it.
            size_t position() const noexcept { return _pos; }

            /// Next token without moving. Precondition: has_next().
            std::string_view peek() const noexcept { return _args[_pos]; }

            /// Next token, then advance. Precondition: has_next().
            std::string_view next() noexcept { return _args[_pos++]; }

            /// True if a next token exists and does not look like a flag.
            bool next_is_value() const noexcept {
                return has_next() && (peek().empty() || peek().front() != '-');
            }

        private:
            const std::vector<std::string>& _args;
            size_t _pos;
    };
}
