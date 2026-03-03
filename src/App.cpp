#include "App.hpp"
#include "ClapExceptions.hpp"

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

void clap::App::parse(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        std::string_view token = argv[i];

        if (!starts_with(token, "-"))
            continue;

        auto *arg = find_argument(token);
        if (!arg)
            throw clap::UnknownArgumentException(std::string(token));

        if (arg->takes_value()) {
            if (i + 1 >= argc)
                throw clap::MissingValueException(std::string(token));
            arg->parse(argv[++i]);
        } else {
            arg->parse("");
        }
    }
    for (const auto& arg : _arguments) {
        if (arg->is_required() && !arg->is_set())
            throw clap::MissingRequiredArgumentException(std::string(arg->names()));
    }
}
