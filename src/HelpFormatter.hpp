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
                          const ArgList& options, const ArgList& positionals,
                          std::vector<std::pair<std::string, std::string>> examples,
                          std::string_view footer = "")
                : _name(name), _description(description),
                  _options(options), _positionals(positionals), _examples(examples),
                  _footer(footer) {}

            /// The "Usage: ..." one-liner.
            std::string usage() const;
            /// Full help: usage, description, and aligned option tables.
            std::string help() const;

        private:
            /// Past this many lines the usage stops being a summary, and the
            /// optional options collapse into a single [OPTIONS]. A count of
            /// options would be the wrong measure: six one-letter flags fit on
            /// one line, three long ones do not.
            static constexpr size_t _usage_max_lines = 2;
            /// A name wider than this does not widen the name column; it runs
            /// over instead of pushing every other row right.
            static constexpr size_t _name_max = 24;
            /// Descriptions never start further right than this. A row whose
            /// name and type reach it puts its description on the next line.
            static constexpr size_t _desc_max = 34;
            /// A wrapped usage line continues under its first token, unless
            /// the program name is long enough to push that past this. Half
            /// the line: past that the tokens have less room than the indent.
            static constexpr size_t _usage_indent_max = 40;
            /// Total line width descriptions wrap at. Fixed, not detected: the
            /// terminal's real width costs an ioctl on POSIX and a different
            /// call on Windows, and 80 gets nearly all of the benefit (#41).
            static constexpr size_t _line_width = 80;

            std::string usage_token(const Argument& arg, bool positional) const;
            std::vector<std::string> usage_tokens(bool collapse) const;
            std::vector<std::string> pack(const std::vector<std::string>& tokens,
                                          const std::string& head, size_t indent) const;
            std::string name_col(const Argument& arg) const;
            std::string type_col(const Argument& arg) const;
            std::string annotation(const Argument& arg) const;
            std::string prefix_col(const Argument& arg, size_t name_w) const;
            std::string row(const Argument& arg, size_t name_w, size_t desc_col) const;
            std::vector<std::string> wrap(const std::string& text, size_t width) const;
            std::string table(const ArgList& args, size_t name_w, size_t desc_col) const;
            size_t name_width() const;
            size_t desc_column() const;

            std::string_view _name;
            std::string_view _description;
            const ArgList& _options;
            const ArgList& _positionals;
            std::vector<std::pair<std::string, std::string>> _examples;
            std::string_view _footer;
    };
}
