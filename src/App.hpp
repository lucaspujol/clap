#pragma once

#include "Option.hpp"
#include "Flag.hpp"
#include "Positional.hpp"
#include "MultiOption.hpp"

#include <string>
#include <vector>
#include <memory>

namespace clap {
    class ArgCursor;

    /// The parser. Register arguments, then call parse(argc, argv).
    class App {
        public:
            /// name shows in the usage line; description shows in help.
            App(std::string name, std::string description);

            App(const App&) = delete;
            App& operator=(const App&) = delete;
            App(App&&) = delete;
            App& operator=(App&&) = delete;

            /// Register a value option, e.g. option<int>("-c,--count", "...").
            template<typename T>
            Option<T>& option(std::string names, std::string description) {
                static_assert(OptionValue<T>,
                    "clap: this option's value type is not usable. clap needs to "
                    "parse it from a string (give it operator>> or specialize "
                    "clap::ParseValue<T>) and print its default (give it "
                    "operator<<). Also specialize clap::TypeName<T> for its help "
                    "label -- see examples/custom_type.");
                auto option = std::make_unique<Option<T>>(std::move(names), std::move(description));
                auto &ref = *option;
                add_argument(std::move(option));
                return ref;
            }

            /// Register a boolean flag, e.g. flag("-v,--verbose", "...").
            Flag& flag(std::string names, std::string description) {
                auto flag = std::make_unique<Flag>(std::move(names), std::move(description));
                auto &ref = *flag;
                add_argument(std::move(flag));
                return ref;
            }

            /// Register a repeatable option, e.g. multi_option<std::string>("-t,--tag", "...").
            template<typename T>
            MultiOption<T>& multi_option(std::string names, std::string description) {
                static_assert(Parseable<T>,
                    "clap: this option's value type cannot be parsed from a string. "
                    "Give it operator>> or specialize clap::ParseValue<T> (and "
                    "clap::TypeName<T> for its help label) -- see examples/custom_type.");
                auto opt = std::make_unique<MultiOption<T>>(std::move(names), std::move(description));
                auto& ref = *opt;
                add_argument(std::move(opt));
                return ref;
            }

            /// Register a positional argument, e.g. positional<std::string>("input", "...").
            template<typename T>
            Positional<T>& positional(std::string name, std::string description) {
                static_assert(OptionValue<T>,
                    "clap: this positional's value type is not usable. clap needs to "
                    "parse it from a string (give it operator>> or specialize "
                    "clap::ParseValue<T>) and print its default (give it "
                    "operator<<). Also specialize clap::TypeName<T> for its help "
                    "label -- see examples/custom_type.");
                auto pos = std::make_unique<Positional<T>>(std::move(name), std::move(description));
                auto &ref = *pos;
                _positionals.push_back(std::move(pos));
                return ref;
            }

            /// Remove the built-in -h/--help, freeing those names for your own use.
            App& no_auto_help();
            /// Rename the built-in help flag, e.g. help_flag("-?,--help").
            App& help_flag(std::string names);

            /// Parse argv. Throws a ClapException on error, or HelpRequested on -h.
            void parse(int argc, char **argv);
            /// One-line usage summary string.
            std::string usage() const;

        private:
            std::string _name;
            std::string _description;
            std::vector<std::unique_ptr<Argument>> _arguments;
            std::vector<std::unique_ptr<Argument>> _positionals;
            size_t _positional_idx = 0;
            Flag* _help = nullptr;

            void add_argument(std::unique_ptr<Argument> arg);
            void remove_help();
            Argument* find_argument(std::string_view name);
            static bool starts_with(std::string_view str, std::string_view prefix);

            void handle_positional(std::string_view token);
            void parse_long_equals(std::string_view token);
            void parse_short_cluster(std::string_view token, ArgCursor& cursor);
            void parse_single(std::string_view token, ArgCursor& cursor);
            void check_required() const;

            void print_help() const;
    };
}