#pragma once

#include <exception>
#include <string>

namespace clap {
    /// What kind of error App::parse() recorded. Query via App::error_kind().
    enum class ErrorKind {
        /// A token was passed that matches no registered argument.
        UnknownArgument,
        /// A value-taking argument was given no value.
        MissingValue,
        /// A flag was given a value, e.g. --verbose=1.
        UnexpectedValue,
        /// A value had the wrong format for its type.
        InvalidValue,
        /// A required argument was absent.
        MissingRequiredValue,
    };

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

    /// Base of the errors App::parse() reports. parse() catches these internally
    /// and records them as error state, so they never escape it. They still
    /// surface from Option::get() and friends, where they mean the caller read a
    /// value that was never set.
    class ParseException : public ClapException {
        public:
            ParseException(ErrorKind kind, const std::string& message)
                : ClapException(message), _kind(kind) {}

            /// Which ErrorKind this maps to in App::error_kind().
            ErrorKind kind() const noexcept { return _kind; }

        private:
            ErrorKind _kind;
    };

    /// An argument was passed that was never registered.
    class UnknownArgument : public ParseException {
        public:
            UnknownArgument(const std::string& arg)
                : ParseException(ErrorKind::UnknownArgument,
                                 "Unknown argument: " + arg) {}
    };

    /// A value-taking argument was given no value.
    class MissingValue : public ParseException {
        public:
            MissingValue(const std::string& arg)
                : ParseException(ErrorKind::MissingValue,
                                 "Missing value for argument: " + arg) {}
    };

    /// A flag was given a value, e.g. --verbose=1.
    class UnexpectedValue : public ParseException {
        public:
            UnexpectedValue(const std::string& arg)
                : ParseException(ErrorKind::UnexpectedValue,
                                 "flag '" + arg + "' does not take a value") {}
    };

    /// A required argument was missing.
    class MissingRequiredArgument : public ParseException {
        public:
            MissingRequiredArgument(const std::string& arg)
                : ParseException(ErrorKind::MissingRequiredValue,
                                 "Missing required argument: " + arg) {}
    };

    /// A value had the wrong format for its type.
    class InvalidValue : public ParseException {
        public:
            InvalidValue(const std::string& value, const std::string& arg,
                         const std::string& type)
                : ParseException(ErrorKind::InvalidValue,
                                 "invalid value '" + value + "' for '" + arg + "'"
                                 + (type.empty() ? "" : " (expected " + type + ")")) {}
    };

    /// The parser was set up wrong. Thrown while registering, not while parsing,
    /// so parse() never catches it: it means the program itself is buggy.
    class ConfigError : public ClapException {
        public:
            ConfigError(const std::string& msg)
                : ClapException("Configuration error: " + msg) {}
    };

    /// A raw value failed to parse. Internal signal from a ParseValue
    /// specialization; parse_checked() turns it into an InvalidValue.
    class ParseError : public ClapException {
        public:
            ParseError(const std::string& msg)
                : ClapException("Parse error: " + msg) {}
    };
}
