#include "App.hpp"
#include "ClapExceptions.hpp"
#include <iostream>
#include <iomanip>

clap::App::App(std::string name, std::string description)
    : name(std::move(name)), description(std::move(description)) {}

clap::IArgument* clap::App::find_argument(std::string_view token) {
    for (auto& arg : _arguments)
        if (arg->matches(token)) return arg.get();
    return nullptr;
}

bool clap::App::starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() &&
        str.compare(0, prefix.size(), prefix) == 0;
}

void clap::App::print_help() {
    std::cout << "Usage: " << name << " [options]";
    for (const auto& pos : _positionals)
        std::cout << " <" << pos->names() << ">";
    std::cout << "\n\n" << description << "\n";

    const std::string help_prefix = "  -h,--help";
    size_t max_w = help_prefix.size();
    for (const auto& arg : _arguments)
        max_w = std::max(max_w, arg->prefix().size());
    for (const auto& pos : _positionals)
        max_w = std::max(max_w, pos->prefix().size());
    const size_t col = max_w + 2;

    std::cout << "\nOptions:\n";
    for (const auto& arg : _arguments) {
        std::cout << std::left << std::setw(col) << arg->prefix()
                    << arg->description();
        if (arg->is_required()) std::cout << " (required)";
        std::cout << "\n";
    }
    std::cout << std::left << std::setw(col) << help_prefix
                << "Show this help message\n";

    if (!_positionals.empty()) {
        std::cout << "\nArguments:\n";
        for (const auto& pos : _positionals)
            std::cout << std::left << std::setw(col) << pos->prefix()
                        << pos->description() << "\n";
    }
}


void clap::App::parse(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        std::string_view token = argv[i];

        if (token == "-h" || token == "--help") {
            print_help();
            throw clap::HelpRequested();
        }

        if (!starts_with(token, "-")) {
            if (_positional_idx >= _positionals.size())
                throw clap::UnknownArgument(std::string(token));
            _positionals[_positional_idx++]->parse(token);
            continue;
        }

        // --option=value
        if (starts_with(token, "--")) {
            auto eq = token.find('=');
            if (eq != std::string_view::npos) {
                auto arg_name  = token.substr(0, eq);
                auto arg_value = token.substr(eq + 1);
                auto *arg = find_argument(arg_name);
                if (!arg)
                    throw clap::UnknownArgument(std::string(arg_name));
                if (!arg->takes_value())
                    throw clap::ParseError("flag '" + std::string(arg_name) + "' does not take a value");
                arg->parse(arg_value);
                continue;
            }
        }

        // combined short flags: -vf -> -v -f
        if (!starts_with(token, "--") && token.size() > 2) {
            for (size_t j = 1; j < token.size(); ++j) {
                std::string short_name{'-', token[j]};
                auto *arg = find_argument(short_name);
                if (!arg)
                    throw clap::UnknownArgument(short_name);
                if (arg->takes_value())
                    throw clap::ParseError("cannot combine '" + short_name + "': it takes a value");
                arg->parse("");
            }
            continue;
        }

        auto *arg = find_argument(token);
        if (!arg)
            throw clap::UnknownArgument(std::string(token));

        if (arg->takes_value()) {
            if (i + 1 >= argc)
                throw clap::MissingValue(std::string(token));
            arg->parse(argv[++i]);
        } else {
            arg->parse("");
        }
    }
    for (const auto& arg : _arguments) {
        if (arg->is_required() && !arg->is_set())
            throw clap::MissingRequiredArgument(std::string(arg->names()));
    }
    for (const auto& pos : _positionals) {
        if (pos->is_required() && !pos->is_set())
            throw clap::MissingRequiredArgument(std::string(pos->names()));
    }
}
