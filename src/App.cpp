#include "App.hpp"
#include "ArgCursor.hpp"
#include "ClapExceptions.hpp"
#include "HelpFormatter.hpp"
#include "damerau_osa.hpp"
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace clap::detail {
    inline bool is_name_char(char c) {
        auto u = static_cast<unsigned char>(c);
        return u >= 0x80
            || (u >= '0' && u <= '9')
            || (u >= 'a' && u <= 'z')
            || (u >= 'A' && u <= 'Z');
    }

    inline bool is_long_body(std::string_view body) {
        if (body.empty() || !is_name_char(body[0]))
            return false;
        for (char c : body)
            if (!is_name_char(c) && c != '-' && c != '_')
                return false;
        return true;
    }

    // accepts exactly two formats:
    // -f (single dash, single char)
    // --flag (double dash, long)
    // Anything else -- --f, spaces, --- is out.
    inline bool valid_option_name(std::string_view name) {
        if (name.size() < 2 || name[0] != '-')
            return false;
        if (name[1] == '-') {              // double dash: needs a long body
            auto body = name.substr(2);
            return body.size() >= 2 && is_long_body(body);
        }
        auto body = name.substr(1);        // single dash
        // short: any char but space, dash, and the two the grammar reserves --
        // '/' is the discard sigil, '=' the long-option separator, so '-/' and
        // '-=' would register but could never be routed to.
        if (body.size() == 1)
            return body[0] != '-' && body[0] != '/' && body[0] != '=' &&
                   !std::isspace(static_cast<unsigned char>(body[0]));
        return false;
    }
}

inline clap::App::App(std::string name, std::string description)
    : _name(std::move(name)), _description(std::move(description)) {
}

inline void clap::App::add_argument(std::unique_ptr<Argument> arg) {
    if (arg->raw_names().empty())
        throw clap::ConfigError(arg->location(),
            "argument registered with no valid name");
    const auto& names = arg->raw_names();
    for (auto it = names.begin(); it != names.end(); ++it) {
        const auto& n = *it;
        if (!clap::detail::valid_option_name(n))
            throw clap::ConfigError(arg->location(),
                "invalid option name '" + n + "' (expected -f or --flag)");
        if (std::find(names.begin(), it, n) != it)
            throw clap::ConfigError(arg->location(),
                "redeclaration of flag " + n);
        for (const auto& existing : _arguments)
            if (existing->matches(n))
                throw clap::ConfigError(arg->location(),
                    "redeclaration of flag " + n, existing->location());
    }
    _arguments.push_back(std::move(arg));
}

inline void clap::App::add_positional(std::unique_ptr<Argument> pos) {
    pos->mark_positional();
    const auto& names = pos->raw_names();
    if (names.size() != 1)
        throw clap::ConfigError(pos->location(),
            "positional registered with an empty name or a comma (a positional has exactly one name)"
    );
    const auto& name = names.front();
    if (!clap::detail::is_long_body(name))
        throw clap::ConfigError(pos->location(),
            "invalid positional name '" + name + "' (letters, digits, '-' and '_', "
            "starting with a letter or a digit)");
    if (!_positionals.empty() && _positionals.back()->is_multi())
        throw clap::ConfigError(pos->location(),
            "positional '" + name + "' declared after variadic positional '"
            + _positionals.back()->raw_names().front() + "' (a variadic must be last)",
            _positionals.back()->location());
    for (const auto& existing : _positionals) {
        if (existing->matches(name))
            throw clap::ConfigError(pos->location(),
                "redeclaration of positional " + name, existing->location());
    }
    _positionals.push_back(std::move(pos));
}

inline clap::Argument* clap::App::find_argument(std::string_view token) {
    for (auto& arg : _arguments)
        if (arg->matches(token)) return arg.get();
    return nullptr;
}

inline std::string clap::App::did_you_mean(std::string_view token) const {
    std::vector<std::string> candidates;
    for (const auto& arg : _arguments)
        for (const auto& name : arg->raw_names())
            candidates.push_back(name);

    std::string match = clap::detail::suggest(token, candidates);
    return match.empty() ? "" : "did you mean '" + match + "'?";
}

inline bool clap::App::starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() &&
        str.compare(0, prefix.size(), prefix) == 0;
}

inline std::string clap::App::help() const {
    return HelpFormatter(
        _name,
        _description,
        _arguments,
        _positionals,
        _examples,
        _footer,
        _footer_wrap
    ).help();
}

inline std::string clap::App::usage() const {
    return HelpFormatter(
        _name,
        _description,
        _arguments,
        _positionals,
        _examples,
        _footer,
        _footer_wrap
    ).usage();
}

inline void clap::App::dispatch(std::string_view token, ArgCursor& cursor, size_t index) {
    // a bare "-" is the POSIX name for stdin: a value, not a flag.
    if (_positional_mode || !starts_with(token, "-") || token == "-") {
        handle_positional(token, index);
        return;
    }
    std::string_view original = token;
    size_t dashes = starts_with(token, "--") ? 2 : 1;
    bool discard = token.size() > dashes && token[dashes] == '/';
    std::string clean;
    if (discard) {
        clean = std::string(token.substr(0, dashes)) + std::string(token.substr(dashes + 1));
        token = clean;
    }
    // "--=value", "--/", "-/": nothing but dashes left of the '='. Report the
    // token the user typed, not the truncated name.
    if (token.substr(0, token.find('=')).size() <= dashes)
        throw clap::UnknownArgument(std::string(original));

    if (starts_with(token, "--") && token.find("=") != std::string_view::npos)
        parse_long_equals(token, discard);
    else if (!starts_with(token, "--") && token.size() > 2)
        parse_short_cluster(token, cursor, discard);
    else
        parse_single(token, cursor, discard);
}

inline bool clap::App::parse(int argc, char **argv) {
    return parse(std::vector<std::string>(argv, argv + argc));
}

inline bool clap::App::parse(const std::vector<std::string>& args) {
    reset();
    ArgCursor cursor(args);
    std::vector<Failure> failures;

    while (cursor.has_next()) {
        size_t index = cursor.position();
        try {
            std::string_view token = cursor.next();
            if (token == "--" && !_positional_mode) {
                _positional_mode = true;
                continue;
            }
            dispatch(token, cursor, index);
        } catch (const clap::ParseException& e) {
            failures.push_back({index, e.kind(), e.what()});
        }
    }

    // Runs even after a failure, so the values that did parse still fill in.
    assign_positionals(failures);

    for (auto& arg : _arguments) {
        try {
            arg->resolve_env();
        } catch (const clap::ParseException& e) {
            failures.push_back({after_argv, e.kind(), e.what()});
        }
    }

    if (failures.empty()) {
        try {
            check_required();
        } catch (const clap::ParseException& e) {
            failures.push_back({after_argv, e.kind(), e.what()});
        }
    }


    if (!failures.empty()) {
        const Failure& first = *std::min_element(failures.begin(), failures.end(),
            [](const Failure& a, const Failure& b) { return a.index < b.index; });
        _error = "Error: " + first.message + "\n" + usage() + "\n";
        _error_kind = first.kind;
        return false;
    }
    _error.clear();
    _error_kind = clap::ErrorKind::OK;
    return true;
}

inline void clap::App::reset() noexcept {
    _positional_tokens.clear();
    _positional_mode = false;
    _error.clear();
    _error_kind = clap::ErrorKind::OK;
    for (auto& a : _arguments)   a->reset();
    for (auto& p : _positionals) p->reset();
}

inline void clap::App::handle_positional(std::string_view token, size_t index) {
    _positional_tokens.emplace_back(index, std::string(token));
}

inline void clap::App::feed(Argument& pos, const PositionalToken& token,
                            std::vector<Failure>& failures) const {
    try {
        pos.parse(token.second);
    } catch (const clap::ParseException& e) {
        failures.push_back({token.first, e.kind(), e.what()});
    }
}

inline void clap::App::assign_positionals(std::vector<Failure>& failures) {
    // A required variadic needs one token like any other required positional;
    // the rest of what it eats comes out of the surplus.
    size_t required = 0;
    for (const auto& pos : _positionals)
        if (pos->is_required())
            ++required;

    const size_t total = _positional_tokens.size();
    size_t surplus = total > required ? total - required : 0;
    size_t next = 0;

    for (auto& pos : _positionals) {
        if (pos->is_multi()) {         // variadic: always last, so it takes everything left
            while (next < total)
                feed(*pos, _positional_tokens[next++], failures);
            break;
        }
        if (next >= total)
            break;
        if (!pos->is_required()) {     // an optional only eats out of the surplus
            if (surplus == 0)
                continue;
            --surplus;
        }
        feed(*pos, _positional_tokens[next++], failures);
    }

    if (next < total) {
        const auto& [index, token] = _positional_tokens[next];
        failures.push_back({index, clap::ErrorKind::UnknownArgument,
                            "Unknown argument: " + token});
    }
}

// --option=value
inline void clap::App::parse_long_equals(std::string_view token, bool discard) {
    auto eq = token.find('=');
    auto arg_name  = token.substr(0, eq);
    auto arg_value = token.substr(eq + 1);
    auto *arg = find_argument(arg_name);
    if (!arg)
        throw clap::UnknownArgument(std::string(arg_name), did_you_mean(arg_name));
    if (!arg->takes_value())
        throw clap::UnexpectedValue(std::string(arg_name));
    arg->parse(arg_value, discard);
}

// short cluster: -vf, -c10, -c-5, -vc 10
inline void clap::App::parse_short_cluster(std::string_view token, ArgCursor& cursor, bool discard) {
    for (size_t j = 1; j < token.size(); ++j) {
        std::string short_name{'-', token[j]};
        auto *arg = find_argument(short_name);
        if (!arg)
            throw clap::UnknownArgument(short_name, did_you_mean(short_name));
        if (!arg->takes_value() && j + 1 < token.size() && token[j + 1] == '=')
            throw clap::UnexpectedValue(short_name);
        if (arg->takes_value()) {
            auto attached = token.substr(j + 1);
            if (!attached.empty()) {
                // '=' is a long-option separator; a short option attaches directly.
                if (attached.front() == '=')
                    throw clap::InvalidValue(std::string(attached), short_name,
                        std::string(arg->type_name()),
                        "short options take the value attached: '" + short_name
                        + std::string(attached.substr(1)) + "'");
                arg->parse(attached, discard);
            }
            else if (cursor.next_is_value())
                arg->parse(cursor.next(), discard);
            else
                throw clap::MissingValue(short_name);
            return;
        }
        arg->parse("", discard);
    }
}

inline void clap::App::parse_single(std::string_view token, ArgCursor& cursor, bool discard) {
    auto *arg = find_argument(token);
    if (!arg)
        throw clap::UnknownArgument(std::string(token), did_you_mean(token));

    if (arg->takes_value()) {
        if (!cursor.next_is_value())
            throw clap::MissingValue(std::string(token));
        arg->parse(cursor.next(), discard);
    } else {
        arg->parse("", discard);
    }
}

inline void clap::App::check_required() const {
    for (const auto& arg : _arguments)
        if (arg->is_required() && !arg->is_set())
            throw clap::MissingRequiredArgument(std::string(arg->names()));
    for (const auto& pos : _positionals)
        if (pos->is_required() && !pos->is_set())
            throw clap::MissingRequiredArgument(std::string(pos->names()));
}
