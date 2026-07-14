#pragma once

#include <string_view>

namespace clap {
    class ArgCursor {
        public:
            ArgCursor(int argc, char** argv) noexcept
                : _argc(argc), _argv(argv), _pos(1) {}

            bool has_next() const noexcept { return _pos < _argc; }

            // next token without moving. precondition: has_next().
            std::string_view peek() const noexcept { return _argv[_pos]; }

            // next token, then advance. precondition: has_next().
            std::string_view next() noexcept { return _argv[_pos++]; }

            bool next_is_value() const noexcept {
                return has_next() && (peek().empty() || peek().front() != '-');
            }

        private:
            int _argc;
            char** _argv;
            int _pos;
    };
}
