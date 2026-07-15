#pragma once

#include <exception>
#include <string>

namespace clap {
    /// Base of every clap error. Catch this to handle them all.
    class ClapException : public std::exception {
        public:
            ClapException(const std::string& message) : _message(message) {}

            const char* what() const noexcept override {
                return _message.c_str();
            }

        private:
            std::string _message;
    };

    /// An argument was passed that was never registered.
    class UnknownArgument : public ClapException {
        public:
            UnknownArgument(const std::string& arg)
                : ClapException("Unknown argument: " + arg) {}
    };

    /// A value-taking argument was given no value.
    class MissingValue : public ClapException {
        public:
            MissingValue(const std::string& arg)
                : ClapException("Missing value for argument: " + arg) {}
    };

    /// A required argument was missing.
    class MissingRequiredArgument : public ClapException {
        public:
            MissingRequiredArgument(const std::string& arg)
                : ClapException("Missing required argument: " + arg) {}
    };

    /// The parser was set up wrong. Thrown while registering, not while parsing.
    class ConfigError : public ClapException {
        public:
            ConfigError(const std::string& msg)
                : ClapException("Configuration error: " + msg) {}
    };

    /// Thrown when help is requested. Not a failure: catch it and exit 0.
    class HelpRequested : public ClapException {
        public:
            HelpRequested() : ClapException("Help requested") {}
    };

    /// A raw value failed to parse. Usually surfaces to the user as InvalidValue.
    class ParseError : public ClapException {
        public:
            ParseError(const std::string& msg)
                : ClapException("Parse error: " + msg) {}
    };

    /// A value had the wrong format for its type.
    class InvalidValue : public ClapException {
        public:
            InvalidValue(const std::string& value, const std::string& arg,
                         const std::string& type)
                : ClapException("invalid value '" + value + "' for '" + arg + "'"
                    + (type.empty() ? "" : " (expected " + type + ")")) {}
    };
}
