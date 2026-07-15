// A required option next to an optional one with a default.
// Try: ./required_optional -p 8080   |   (omit -p to see the error)
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Required vs optional");
    app.help_flag("--help");

    auto& port = app.option<int>("-p,--port", "Port").required();
    auto& host = app.option<std::string>("-h,--host", "Host").default_value("localhost");

    try {
        app.parse(argc, argv);
    } catch (const clap::HelpRequested&) {
        return 0;
    } catch (const clap::ClapException& e) {
        std::cerr << app.usage() << "\nError: " << e.what() << "\n";
        return 1;
    }

    std::cout << host.get() << ":" << port.get() << "\n";
    return 0;
}
