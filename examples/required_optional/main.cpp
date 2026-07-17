// A required option next to an optional one with a default.
// Try: ./required_optional -p 8080   |   (omit -p to see the error)
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Required vs optional");

    // -h is taken by --host below, so the help flag is --help only.
    auto& help = app.flag("--help", "Show this help message");
    auto& port = app.option<int>("-p,--port", "Port").required();
    auto& host = app.option<std::string>("-h,--host", "Host").default_value("localhost");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help(); return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::cout << host.get() << ":" << port.get() << "\n";
    return 0;
}
