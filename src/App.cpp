#include "App.hpp"
#include "ArgCursor.hpp"
#include "ClapExceptions.hpp"
#include "HelpFormatter.hpp"
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace {
    bool is_long_body(std::string_view body) {
        if (body.empty() || !std::isalnum(static_cast<unsigned char>(body[0])))
            return false;
        for (char c : body)
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_')
                return false;
        return true;
    }

    // accepts exactly three formats:
    // -f (single dash, single char)
    // -flag (single dash, long)
    // --flag (double dash, long)
    // Anything else -- --f, spaces, --- is out.
    bool valid_option_name(std::string_view name) {
        if (name.size() < 2 || name[0] != '-')
            return false;
        if (name[1] == '-') {              // double dash: needs a long body
            auto body = name.substr(2);
            return body.size() >= 2 && is_long_body(body);
        }
        auto body = name.substr(1);        // single dash
        if (body.size() == 1)              // short: any char but space or dash
            return body[0] != '-' && !std::isspace(static_cast<unsigned char>(body[0]));
        return is_long_body(body);         // single-dash long
    }
}

clap::App::App(std::string name, std::string description)
    : _name(std::move(name)), _description(std::move(description)) {
}

void clap::App::add_argument(std::unique_ptr<Argument> arg) {
    if (arg->raw_names().empty())
        throw clap::ConfigError("argument registered with no valid name");
    for (const auto& n : arg->raw_names()) {
        if (!valid_option_name(n))
            throw clap::ConfigError("invalid option name '" + n +
                "' (expected -f, -flag or --flag)");
        for (const auto& existing : _arguments)
            if (existing->matches(n))
                throw clap::ConfigError("duplicate option name: " + n);
    }
    _arguments.push_back(std::move(arg));
}

clap::Argument* clap::App::find_argument(std::string_view token) {
    for (auto& arg : _arguments)
        if (arg->matches(token)) return arg.get();
    return nullptr;
}

bool clap::App::starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() &&
        str.compare(0, prefix.size(), prefix) == 0;
}

std::string clap::App::help() const {
    return HelpFormatter(_name, _description, _arguments, _positionals).help();
}

std::string clap::App::usage() const {
    return HelpFormatter(_name, _description, _arguments, _positionals).usage();
}

void clap::App::dispatch(std::string_view token, ArgCursor& cursor) {
    if (!starts_with(token, "-")) {
        handle_positional(token);
        return;
    }
    size_t dashes = starts_with(token, "--") ? 2 : 1;
    bool discard = token.size() > dashes && token[dashes] == '/';
    std::string clean;
    if (discard) {
        clean = std::string(token.substr(0, dashes)) + std::string(token.substr(dashes + 1));
        token = clean;
    }

    if (starts_with(token, "--") && token.find("=") != std::string_view::npos)
        parse_long_equals(token, discard);
    else if (!starts_with(token, "--") && token.size() > 2)
        parse_short_cluster(token, cursor, discard);
    else
        parse_single(token, cursor, discard);
}

bool clap::App::parse(int argc, char **argv) {
    ArgCursor cursor(argc, argv);
    std::optional<clap::ParseException> failure;

    while (cursor.has_next()) {
        try {
            dispatch(cursor.next(), cursor);
        } catch (const clap::ParseException& e) {
            if (!failure)
                failure = e;
        }
    }

    if (!failure) {
        try {
            check_required();
        } catch (const clap::ParseException& e) {
            failure = e;
        }
    }

    if (failure) {
        _error = "Error: " + std::string(failure->what()) + "\n" + usage() + "\n";
        _error_kind = failure->kind();
        return false;
    }
    _error.clear();
    return true;
}

void clap::App::handle_positional(std::string_view token) {
    if (_positional_idx >= _positionals.size())
        throw clap::UnknownArgument(std::string(token));
    _positionals[_positional_idx++]->parse(token);
}

// --option=value
void clap::App::parse_long_equals(std::string_view token, bool discard) {
    auto eq = token.find('=');
    auto arg_name  = token.substr(0, eq);
    auto arg_value = token.substr(eq + 1);
    auto *arg = find_argument(arg_name);
    if (!arg)
        throw clap::UnknownArgument(std::string(arg_name));
    if (!arg->takes_value())
        throw clap::UnexpectedValue(std::string(arg_name));
    arg->parse(arg_value, discard);
}

// short cluster: -vf, -c10, -c-5, -vc 10
void clap::App::parse_short_cluster(std::string_view token, ArgCursor& cursor, bool discard) {
    for (size_t j = 1; j < token.size(); ++j) {
        std::string short_name{'-', token[j]};
        auto *arg = find_argument(short_name);
        if (!arg)
            throw clap::UnknownArgument(short_name);
        if (arg->takes_value()) {
            auto attached = token.substr(j + 1);
            if (!attached.empty())
                arg->parse(attached, discard);
            else if (cursor.next_is_value())
                arg->parse(cursor.next(), discard);
            else
                throw clap::MissingValue(short_name);
            return;
        }
        arg->parse("", discard);
    }
}

void clap::App::parse_single(std::string_view token, ArgCursor& cursor, bool discard) {
    auto *arg = find_argument(token);
    if (!arg)
        throw clap::UnknownArgument(std::string(token));

    if (arg->takes_value()) {
        if (!cursor.next_is_value())
            throw clap::MissingValue(std::string(token));
        arg->parse(cursor.next(), discard);
    } else {
        arg->parse("", discard);
    }
}

void clap::App::check_required() const {
    for (const auto& arg : _arguments)
        if (arg->is_required() && !arg->is_set())
            throw clap::MissingRequiredArgument(std::string(arg->names()));
    for (const auto& pos : _positionals)
        if (pos->is_required() && !pos->is_set())
            throw clap::MissingRequiredArgument(std::string(pos->names()));
}
