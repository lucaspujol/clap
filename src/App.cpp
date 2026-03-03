#include "App.hpp"
#include <iostream>


clap::App::App(std::string name, std::string description) : name(name), description(description) {}



void clap::App::parse(int argc, char **argv) {

    for (int i = 1; i < argc; ++i) {
        std::string_view token = argv[i];

        if (!starts_with(token, "-")) {
            continue;
        }
        // handle --option=value
        auto *arg = find_argument(token);
        if (!arg) {
            std::cerr << "Unknown argument: " << token << std::endl;
            exit(84);
        }
        if (arg->takes_value())
            arg->parse(argv[++i]);
        else
            arg->parse("");
    }
    for (const auto& arg : _arguments) {
        if (arg->is_required() && !arg->is_set()) {
            std::cerr << "Missing required argument: " << arg->helpLine() << std::endl;
            exit(84);
        }
    }
}
