#pragma once

#include "Option.hpp"
#include "Flag.hpp"
#include "Positional.hpp"
#include "MultiOption.hpp"

#include <string>
#include <vector>
#include <memory>

namespace clap {
    class App {
        public:
            App(std::string name, std::string description);

            App(const App&) = delete;
            App& operator=(const App&) = delete;
            App(App&&) = delete;
            App& operator=(App&&) = delete;

            template<typename T>
            Option<T>& option(std::string names, std::string description) {
                auto option = std::make_unique<Option<T>>(std::move(names), std::move(description));
                auto &ref = *option;
                add_argument(std::move(option));
                return ref;
            }

            Flag& flag(std::string names, std::string description) {
                auto flag = std::make_unique<Flag>(std::move(names), std::move(description));
                auto &ref = *flag;
                add_argument(std::move(flag));
                return ref;
            }

            template<typename T>
            MultiOption<T>& multi_option(std::string names, std::string description) {
                auto opt = std::make_unique<MultiOption<T>>(std::move(names), std::move(description));
                auto& ref = *opt;
                add_argument(std::move(opt));
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
            std::string usage() const;

        private:
            std::string _name;
            std::string _description;
            std::vector<std::unique_ptr<IArgument>> _arguments;
            std::vector<std::unique_ptr<IArgument>> _positionals;
            size_t _positional_idx = 0;
            Flag* _help = nullptr;

            void add_argument(std::unique_ptr<IArgument> arg);
            IArgument* find_argument(std::string_view name);
            static bool starts_with(std::string_view str, std::string_view prefix);
            void print_help() const;
            static void print_row(const IArgument& arg, size_t col);
    };
}