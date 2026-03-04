#pragma once

#include "Option.hpp"
#include "Flag.hpp"
#include "Positional.hpp"

#include <string>
#include <vector>
#include <memory>

namespace clap {
    class App {
        public:
            App(std::string name, std::string description);

            template<typename T>
            Option<T>& option(std::string names, std::string description) {
                auto option = std::make_unique<Option<T>>(std::move(names), std::move(description));
                auto &ref = *option;
                _arguments.push_back(std::move(option));
                return ref;
            }

            Flag& flag(std::string names, std::string description) {
                auto flag = std::make_unique<Flag>(std::move(names), std::move(description));
                auto &ref = *flag;
                _arguments.push_back(std::move(flag));
                return ref;
            }

            template<typename T>
            Positional<T>& positional(std::string name, std::string description) {
                auto pos = std::make_unique<Positional<T>>(std::move(name), std::move(description));
                auto &ref = *pos;
                _positionals.push_back(std::move(pos));
                return ref;
            }

            void parse(int argc, char **argv);

        private:
            std::string name;
            std::string description;
            std::vector<std::unique_ptr<IArgument>> _arguments;
            std::vector<std::unique_ptr<IArgument>> _positionals;
            size_t _positional_idx = 0;

            IArgument* find_argument(std::string_view name);
            static bool starts_with(std::string_view str, std::string_view prefix);
            void print_help();
    };
}