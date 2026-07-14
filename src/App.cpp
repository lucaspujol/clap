#include "App.hpp"
#include "ArgCursor.hpp"
#include "ClapExceptions.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

clap::App::App(std::string name, std::string description)
    : _name(std::move(name)), _description(std::move(description)) {
    _help = &flag("-h,--help", "Show this help message");
}

void clap::App::add_argument(std::unique_ptr<IArgument> arg) {
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

clap::IArgument* clap::App::find_argument(std::string_view token) {
    for (auto& arg : _arguments)
        if (arg->matches(token)) return arg.get();
    return nullptr;
}

bool clap::App::starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() &&
        str.compare(0, prefix.size(), prefix) == 0;
}

std::string clap::App::usage() const {
    std::ostringstream oss;
    oss << "Usage: " << _name;
    for (const auto& arg : _arguments)
        oss << " " << arg->usage_token();
    for (const auto& pos : _positionals)
        oss << " " << pos->usage_token();
    return oss.str();
}

void clap::App::print_help() const {
    std::cout << usage() << "\n\n" << _description << "\n";

    size_t max_w = 0;
    for (const auto& arg : _arguments)
        max_w = std::max(max_w, arg->prefix().size());
    for (const auto& pos : _positionals)
        max_w = std::max(max_w, pos->prefix().size());
    const size_t col = max_w + 2;

    std::cout << "\nOptions:\n";
    for (const auto& arg : _arguments)
        print_row(*arg, col);

    if (!_positionals.empty()) {
        std::cout << "\nArguments:\n";
        for (const auto& pos : _positionals)
            print_row(*pos, col);
    }
}

void clap::App::print_row(const IArgument& arg, size_t col) {
    std::cout << std::left << std::setw(col) << arg.prefix() << arg.description();
    if (arg.is_required())
        std::cout << " (required)";
    else if (!arg.default_str().empty())
        std::cout << " (default: " << arg.default_str() << ")";
    std::cout << "\n";
}


void clap::App::parse(int argc, char **argv) {
    ArgCursor cursor(argc, argv);

    while (cursor.has_next()) {
        std::string_view token = cursor.next();

        if (!starts_with(token, "-"))
            handle_positional(token);
        else if (starts_with(token, "--") && token.find('=') != std::string_view::npos)
            parse_long_equals(token);
        else if (!starts_with(token, "--") && token.size() > 2)
            parse_short_cluster(token, cursor);
        else
            parse_single(token, cursor);
    }

    if (_help && *_help) {
        print_help();
        throw clap::HelpRequested();
    }

    check_required();
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
        throw clap::ParseError("flag '" + std::string(arg_name) + "' does not take a value");
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
