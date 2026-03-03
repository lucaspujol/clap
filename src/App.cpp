#include "App.hpp"
#include <iostream>
#include "ClapExceptions.hpp"

clap::App::App(std::string name, std::string description) : name(name), description(description) {}

void clap::App::parse(int argc, char **argv) {

    for (int i = 1; i < argc; ++i) {
        std::string_view token = argv[i];

        if (!starts_with(token, "-")) {
            continue;
        }
        // handle --option=value
        auto *arg = find_argument(token);
        if (!arg)
            throw clap::UnknownArgumentException(std::string(token));
            
        if (arg->takes_value()) {
            if (i + 1 >= argc)
                throw clap::MissingValueException(std::string(token));
            arg->parse(argv[++i]);
        } else
            arg->parse("");
    }
    for (const auto& arg : _arguments) {
        if (arg->is_required() && !arg->is_set())
            throw clap::MissingRequiredArgumentException(arg->helpLine());
    }
}
