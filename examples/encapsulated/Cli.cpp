#include "Cli.hpp"

#include <iostream>

std::optional<int> Cli::parse(int argc, char** argv) {
    bool ok = _app.parse(argc, argv);

    if (_help) {
        std::cout << _app.help();
        return 0;
    }
    if (!ok) {
        std::cerr << _app.error();
        return 84;
    }
    return std::nullopt;
}
