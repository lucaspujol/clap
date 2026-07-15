#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace clap {
    class Argument;

    /// Builds the usage line and full help text from an App's arguments.
    class HelpFormatter {
        public:
            using ArgList = std::vector<std::unique_ptr<Argument>>;

            HelpFormatter(std::string_view name, std::string_view description,
                          const ArgList& options, const ArgList& positionals)
                : _name(name), _description(description),
                  _options(options), _positionals(positionals) {}

            /// The "Usage: ..." one-liner.
            std::string usage() const;
            /// Full help: usage, description, and aligned option tables.
            std::string help() const;

        private:
            std::string usage_token(const Argument& arg, bool positional) const;
            std::string name_col(const Argument& arg) const;
            std::string type_col(const Argument& arg) const;
            std::string annotation(const Argument& arg) const;
            size_t name_width() const;
            size_t type_width() const;

            std::string_view _name;
            std::string_view _description;
            const ArgList& _options;
            const ArgList& _positionals;
    };
}
