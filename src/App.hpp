#pragma once

#include "Option.hpp"
#include "Flag.hpp"
#include "Positional.hpp"
#include "MultiOption.hpp"
#include "ClapExceptions.hpp"

#include <string>
#include <vector>
#include <memory>
#include <source_location>

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
            Option<T>& option(std::string names, std::string description,
                              std::source_location loc = std::source_location::current()) {
                static_assert(OptionValue<T>,
                    "clap: this option's value type is not usable. clap needs to "
                    "parse it from a string (give it operator>> or specialize "
                    "clap::ParseValue<T>) and print its default (give it "
                    "operator<<). Also specialize clap::TypeName<T> for its help "
                    "label -- see examples/custom_type.");
                auto option = std::make_unique<Option<T>>(std::move(names), std::move(description));
                option->set_location(loc);
                auto &ref = *option;
                add_argument(std::move(option));
                return ref;
            }

            /// Register a boolean flag, e.g. flag("-v,--verbose", "...").
            Flag& flag(std::string names, std::string description,
                       std::source_location loc = std::source_location::current()) {
                auto flag = std::make_unique<Flag>(std::move(names), std::move(description));
                flag->set_location(loc);
                auto &ref = *flag;
                add_argument(std::move(flag));
                return ref;
            }

            /// Register a repeatable option, e.g. multi_option<std::string>("-t,--tag", "...").
            template<typename T>
            MultiOption<T>& multi_option(std::string names, std::string description,
                                         std::source_location loc = std::source_location::current()) {
                static_assert(Parseable<T>,
                    "clap: this option's value type cannot be parsed from a string. "
                    "Give it operator>> or specialize clap::ParseValue<T> (and "
                    "clap::TypeName<T> for its help label) -- see examples/custom_type.");
                auto opt = std::make_unique<MultiOption<T>>(std::move(names), std::move(description));
                opt->set_location(loc);
                auto& ref = *opt;
                add_argument(std::move(opt));
                return ref;
            }

            /// Register a positional argument, e.g. positional<std::string>("input", "...").
            template<typename T>
            Positional<T>& positional(std::string name, std::string description,
                                      std::source_location loc = std::source_location::current()) {
                static_assert(OptionValue<T>,
                    "clap: this positional's value type is not usable. clap needs to "
                    "parse it from a string (give it operator>> or specialize "
                    "clap::ParseValue<T>) and print its default (give it "
                    "operator<<). Also specialize clap::TypeName<T> for its help "
                    "label -- see examples/custom_type.");
                auto pos = std::make_unique<Positional<T>>(std::move(name), std::move(description));
                pos->set_location(loc);
                auto &ref = *pos;
                _positionals.push_back(std::move(pos));
                return ref;
            }

            /// Parse argv. Never throws on bad input; returns true on success,
            /// false if an error was recorded (see error()/error_kind()). It fills
            /// every value it can regardless. Registration still throws ConfigError.
            bool parse(int argc, char **argv);
            /// Full help message.
            std::string help() const;
            /// One-line usage summary string.
            std::string usage() const;

            /// The error text to print (message + usage line), empty when parse() succeeded.
            const std::string& error() const noexcept { return _error; }
            /// Which error parse() recorded. Precondition: parse() returned false.
            ErrorKind error_kind() const noexcept { return _error_kind; }

        private:
            std::string _name;
            std::string _description;
            std::vector<std::unique_ptr<Argument>> _arguments;
            std::vector<std::unique_ptr<Argument>> _positionals;
            size_t _positional_idx = 0;
            std::string _error;
            ErrorKind _error_kind{};
            bool _positional_mode = false;

            void add_argument(std::unique_ptr<Argument> arg);
            Argument* find_argument(std::string_view name);
            static bool starts_with(std::string_view str, std::string_view prefix);

            void dispatch(std::string_view token, ArgCursor& cursor);
            void handle_positional(std::string_view token);
            void check_required() const;

            void parse_long_equals(std::string_view token, bool discard);
            void parse_short_cluster(std::string_view token, ArgCursor& cursor, bool discard);
            void parse_single(std::string_view token, ArgCursor& cursor, bool discard);
    };
}