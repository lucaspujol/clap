#pragma once

#include <string>
#include <sstream>
#include <source_location>
#include <vector>

namespace clap {
    /// Base of every argument type. Holds names and description;
    /// subclasses decide how to parse and store a value.
    class Argument {
        public:
            Argument(std::string names, std::string description)
                : _names_raw(std::move(names)),
                  _names(split(_names_raw, ',')),
                  _description(std::move(description)) {}
            virtual ~Argument() = default;

            /// Consume a raw token as this argument's value. When discard is
            /// true the token is still validated but the result is thrown away,
            /// leaving the argument unset (used by the -/flag override syntax).
            virtual void parse(std::string_view value, bool discard = false) = 0;
            /// Type label for help, e.g. "int". Empty for flags.
            virtual std::string_view type_name() const = 0;

            /// True once a value has been parsed.
            virtual bool is_set() const noexcept = 0;
            /// True if this argument consumes a value token.
            virtual bool takes_value() const noexcept = 0;
            /// True if it can be repeated to collect a list.
            virtual bool is_multi() const noexcept { return false; }

            /// Rendered default value for help, or empty if none.
            virtual std::string default_str() const { return ""; }

            /// Constraints a validator wants stated next to the description,
            /// e.g. "must exist". Empty for flags, which take no validator.
            virtual const std::vector<std::string>& hints() const {
                static const std::vector<std::string> none;
                return none;
            }

            /// Environment variable this argument falls back to, or empty.
            virtual std::string env_key() const { return ""; }
            /// Apply the env-var fallback when still unset. May throw
            virtual void resolve_env() {}

            std::string_view names() const noexcept { return _names_raw; }
            std::string_view description() const noexcept { return _description; }
            virtual bool is_required() const noexcept { return _required; }
            bool is_hidden() const noexcept { return _hidden; }

            /// The help section this argument was filed under, empty for the
            /// default one.
            const std::string& group() const noexcept { return _group; }

            /// True if App put this in the positional list rather than the
            /// option list. The base class cannot tell on its own, so App sets
            /// it. But group() needs to know, and so would anything else that
            /// is only meaningful on one of the two.
            bool is_positional() const noexcept { return _positional; }

            const std::vector<std::string>& raw_names() const noexcept { return _names; }

            /// Shortest registered name, preferred for the usage summary (e.g. "-v").
            std::string_view primary_name() const noexcept {
                std::string_view best;
                for (const auto& n : _names)
                    if (best.empty() || n.size() < best.size())
                        best = n;
                return best;
            }

            /// Where this argument was registered, for diagnostics. Set by App
            /// right after construction, so it points at the caller's site.
            void set_location(const std::source_location& loc) noexcept { _loc = loc; }

            /// Called by App::add_positional, right after construction and
            /// before the caller gets a chance to chain anything.
            void mark_positional() noexcept { _positional = true; }

            const std::source_location& location() const noexcept { return _loc; }

            /// True if token matches one of this argument's names.
            bool matches(std::string_view name) const {
                for (const auto& arg_name : _names) {
                    if (arg_name == name)
                        return true;
                }
                return false;
            }

            virtual void reset() noexcept = 0;

        protected:
            void set_required() noexcept { _required = true; }
            void set_hidden()   noexcept { _hidden   = true; }
            void set_group(std::string group) { _group = std::move(group); }

        private:
            std::string _names_raw;
            std::vector<std::string> _names;
            std::string _description;
            std::string _group;
            bool _required = false;
            bool _hidden = false;
            bool _positional = false;
            std::source_location _loc{};

            static std::vector<std::string> split(const std::string &str, char delimiter) {
                std::vector<std::string> tokens;
                std::istringstream iss(str);
                std::string token;

                while (std::getline(iss, token, delimiter)) {
                    token.erase(0, token.find_first_not_of(" \t\n\r\f\v"));
                    token.erase(token.find_last_not_of(" \t\n\r\f\v") + 1);
                    if (!token.empty())
                        tokens.push_back(token);
                }
                return tokens;
            }
    };
}
