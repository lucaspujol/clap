// Rename the help flag with help_flag() so -h is free for your own use.
// Try: ./custom_help -h   (sets host)   |   ./custom_help '-?'   (shows help)
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Custom help flag");

    app.help_flag("-?,--help");
    auto& host = app.flag("-h,--host", "Server host");

    try {
        app.parse(argc, argv);
    } catch (const clap::HelpRequested&) {
        return 0;
    } catch (const clap::ClapException& e) {
        std::cerr << app.usage() << "\nError: " << e.what() << "\n";
        return 1;
    }

    std::cout << "host = " << (host ? "set" : "unset") << "\n";
    return 0;
}
