#include "App.hpp"
#include "ArgCursor.hpp"
#include "ClapExceptions.hpp"
#include "HelpFormatter.hpp"
#include <optional>
#include <string>

clap::App::App(std::string name, std::string description)
    : _name(std::move(name)), _description(std::move(description)) {
}

void clap::App::add_argument(std::unique_ptr<Argument> arg) {
    if (arg->raw_names().empty())
        throw clap::ConfigError("argument registered with no valid name");
    for (const auto& n : arg->raw_names()) {
        if (n[0] != '-')
            throw clap::ConfigError("option name must start with '-': " + n);
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
    if (!starts_with(token, "-"))
        handle_positional(token);
    else if (starts_with(token, "--") && token.find('=') != std::string_view::npos)
        parse_long_equals(token);
    else if (!starts_with(token, "--") && token.size() > 2)
        parse_short_cluster(token, cursor);
    else
        parse_single(token, cursor);
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
void clap::App::parse_long_equals(std::string_view token) {
    auto eq = token.find('=');
    auto arg_name  = token.substr(0, eq);
    auto arg_value = token.substr(eq + 1);
    auto *arg = find_argument(arg_name);
    if (!arg)
        throw clap::UnknownArgument(std::string(arg_name));
    if (!arg->takes_value())
        throw clap::UnexpectedValue(std::string(arg_name));
    arg->parse(arg_value);
}

// short cluster: -vf, -c10, -c-5, -vc 10
void clap::App::parse_short_cluster(std::string_view token, ArgCursor& cursor) {
    for (size_t j = 1; j < token.size(); ++j) {
        std::string short_name{'-', token[j]};
        auto *arg = find_argument(short_name);
        if (!arg)
            throw clap::UnknownArgument(short_name);
        if (arg->takes_value()) {
            auto attached = token.substr(j + 1);
            if (!attached.empty())
                arg->parse(attached);
            else if (cursor.next_is_value())
                arg->parse(cursor.next());
            else
                throw clap::MissingValue(short_name);
            return;
        }
        arg->parse("");
    }
}

void clap::App::parse_single(std::string_view token, ArgCursor& cursor) {
    auto *arg = find_argument(token);
    if (!arg)
        throw clap::UnknownArgument(std::string(token));

    if (arg->takes_value()) {
        if (!cursor.next_is_value())
            throw clap::MissingValue(std::string(token));
        arg->parse(cursor.next());
    } else {
        arg->parse("");
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
