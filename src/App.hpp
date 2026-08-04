#pragma once

#include "HelpFormatter.hpp"
#include "Option.hpp"
#include "Flag.hpp"
#include "Positional.hpp"
#include "ValueList.hpp"
#include "ClapExceptions.hpp"
#include "Concepts.hpp"

#include <string>
#include <vector>
#include <memory>
#include <source_location>
#include <utility>

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
            ValueList<T>& multi_option(std::string names, std::string description,
                                       std::source_location loc = std::source_location::current()) {
                static_assert(OptionValue<T>,
                    "clap: this option's value type is not usable. clap needs to "
                    "parse it from a string (give it operator>> or specialize "
                    "clap::ParseValue<T>) and print its default (give it "
                    "operator<<). Also specialize clap::TypeName<T> for its help "
                    "label -- see examples/custom_type.");
                auto opt = std::make_unique<ValueList<T>>(std::move(names), std::move(description));
                opt->set_location(loc);
                auto& ref = *opt;
                add_argument(std::move(opt));
                return ref;
            }

            /// Register a variadic positional, e.g. variadic<std::string>("files", "...").
            /// Must be the last positional; greedily collects every remaining token.
            template<typename T>
            ValueList<T>& variadic(std::string name, std::string description,
                                   std::source_location loc = std::source_location::current()) {
                static_assert(OptionValue<T>,
                    "clap: this positional's value type is not usable. clap needs to "
                    "parse it from a string (give it operator>> or specialize "
                    "clap::ParseValue<T>) and print its default (give it "
                    "operator<<). Also specialize clap::TypeName<T> for its help "
                    "label -- see examples/custom_type.");
                auto pos = std::make_unique<ValueList<T>>(std::move(name), std::move(description));
                pos->set_location(loc);
                auto& ref = *pos;
                add_positional(std::move(pos));
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
                add_positional(std::move(pos));
                return ref;
            }

            /// Parse the argument list. args[0] is the program name and is
            /// skipped, exactly as argv[0] is.
            /// Never throws on bad input; returns true on success,
            /// false if an error was recorded (see error()/error_kind()). It fills
            /// every value it can regardless. Registration still throws ConfigError.
            ///
            /// Two special forms are recognised:
            /// - "--" on its own: every token after it is treated as positional,
            ///   even ones that look like flags.
            /// - a "/" right after the dashes (e.g. -/v, --/count=3): parses and
            ///   validates the argument but discards its value, leaving it unset.
            bool parse(const std::vector<std::string>& args);

            /// Same, from what main() receives. Copies argv into a vector and
            /// calls the overload above.
            bool parse(int argc, char **argv);

            /// called by parse on entry. prevents weird/stale state when calling parse
            /// multiple times on the same app. We dont prevent re-entry to let the user
            /// keep control of his app state.
            void reset() noexcept;

            /// Full help message.
            std::string help() const;
            /// One-line usage summary string.
            std::string usage() const;

            /// Add an example usage line to the help message. The description is optional.
            void example(const std::string& example, const std::string& description = "", std::source_location loc = std::source_location::current()) {
                if (example.empty()) throw clap::ConfigError(loc, "example() cannot be called with an empty string");
                _examples.emplace_back(example, description);
            }

            /// Add a footer to the help message. The footer is printed after the examples.
            /// By default, the footer wraps like the rest of the help text. If you want to
            /// disable this behavior, call app.disable_footer_wrap() before calling this
            /// method.
            /// The footer is optional and can be empty (wont be displayed)
            void footer(const std::string& footer) {
                _footer = footer;
            }

            /// Disable text's wrapping in the footer. I made this so rendering ascii
            /// art in the footer works nicely, and regular footers dont need to be a
            /// properly formatted string.
            void disable_footer_wrap() { _footer_wrap = false; }

            /// The error text to print (message + usage line), empty when parse() succeeded.
            const std::string& error() const noexcept { return _error; }
            /// Which error parse() recorded. ErrorKind::OK before parse() runs and
            /// after a parse that succeeded.
            ErrorKind error_kind() const noexcept { return _error_kind; }

        private:
            std::string _name;
            std::string _description;
            std::vector<std::unique_ptr<Argument>> _arguments;
            std::vector<std::unique_ptr<Argument>> _positionals;

            /// A positional token and the argv slot it came from. Positionals
            /// are collected during the walk and assigned once it ends, so the
            /// slot has to be carried along for error ordering.
            using PositionalToken = std::pair<size_t, std::string>;
            std::vector<PositionalToken> _positional_tokens;

            /// A recorded parse error, tagged with its argv slot.
            struct Failure {
                size_t index;
                ErrorKind kind;
                std::string message;
            };

            /// index for a failure that belongs after the whole walk.
            static constexpr size_t after_argv = static_cast<size_t>(-1);
            std::string _error;
            ErrorKind _error_kind{ErrorKind::OK};
            bool _positional_mode = false;

            std::vector<std::pair<std::string, std::string>> _examples;
            std::string _footer;
            bool _footer_wrap;

            void add_argument(std::unique_ptr<Argument> arg);
            void add_positional(std::unique_ptr<Argument> pos);
            Argument* find_argument(std::string_view name);
            /// "did you mean '--x'?" for an unrecognised token, "" when nothing
            /// registered is close enough.
            std::string did_you_mean(std::string_view token) const;
            static bool starts_with(std::string_view str, std::string_view prefix);

            void dispatch(std::string_view token, ArgCursor& cursor, size_t index);
            void handle_positional(std::string_view token, size_t index);
            /// Hands the collected tokens to the positionals: required ones
            /// first in declaration order, then optionals take what is left
            /// over. Runs once, after the walk.
            void assign_positionals(std::vector<Failure>& failures);
            void feed(Argument& pos, const PositionalToken& token,
                      std::vector<Failure>& failures) const;
            void check_required() const;

            void parse_long_equals(std::string_view token, bool discard);
            void parse_short_cluster(std::string_view token, ArgCursor& cursor, bool discard);
            void parse_single(std::string_view token, ArgCursor& cursor, bool discard);
    };
}
