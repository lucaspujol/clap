#pragma once

#include <string_view>

namespace clap {
    /// Walks argv left to right, one token at a time (skips argv[0]).
    class ArgCursor {
        public:
            ArgCursor(int argc, char** argv) noexcept
                : _argc(argc), _argv(argv), _pos(1) {}

            bool has_next() const noexcept { return _pos < _argc; }

            /// Next token without moving. Precondition: has_next().
            std::string_view peek() const noexcept { return _argv[_pos]; }

            /// Next token, then advance. Precondition: has_next().
            std::string_view next() noexcept { return _argv[_pos++]; }

            /// True if a next token exists and does not look like a flag.
            bool next_is_value() const noexcept {
                return has_next() && (peek().empty() || peek().front() != '-');
            }

        private:
            int _argc;
            char** _argv;
            int _pos;
    };
}
