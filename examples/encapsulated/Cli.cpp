#include "Cli.hpp"

#include <iostream>

std::optional<int> Cli::parse(int argc, char** argv) {
    try {
        _app.parse(argc, argv);
    } catch (const clap::HelpRequested&) {
        return 0;
    } catch (const clap::ClapException& e) {
        std::cerr << _app.usage() << "\nError: " << e.what() << "\n";
        return 84;
    }
    return std::nullopt;
}
