#pragma once

#include <stdexcept>

namespace clap {
    class ClapException : public std::exception {
        public:
            ClapException(const std::string& message) : _message(message) {}

            const char* what() const noexcept override {
                return _message.c_str();
            }

        private:
            std::string _message;
    };

    class UnknownArgument : public ClapException {
        public:
            UnknownArgument(const std::string& arg)
                : ClapException("Unknown argument: " + arg) {}
    };

    class MissingValue : public ClapException {
        public:
            MissingValue(const std::string& arg)
                : ClapException("Missing value for argument: " + arg) {}
    };

    class MissingRequiredArgument : public ClapException {
        public:
            MissingRequiredArgument(const std::string& arg)
                : ClapException("Missing required argument: " + arg) {}
    };

    class ConfigError : public ClapException {
        public:
            ConfigError(const std::string& msg)
                : ClapException("Configuration error: " + msg) {}
    };

    class HelpRequested : public ClapException {
        public:
            HelpRequested() : ClapException("Help requested") {}
    };

    class ParseError : public ClapException {
        public:
            ParseError(const std::string& msg)
                : ClapException("Parse error: " + msg) {}
    };
}
