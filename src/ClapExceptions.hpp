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

    class UnknownArgumentException : public ClapException {
        public:
            UnknownArgumentException(const std::string& arg)
                : ClapException("Unknown argument: " + arg) {}
    };

    class MissingValueException : public ClapException {
        public:
            MissingValueException(const std::string& arg)
                : ClapException("Missing value for argument: " + arg) {}
    };

    class MissingRequiredArgumentException : public ClapException {
        public:
            MissingRequiredArgumentException(const std::string& arg)
                : ClapException("Missing required argument: " + arg) {}
    };
}