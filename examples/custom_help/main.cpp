// Help is just a flag you register — name it whatever you like. Here -h is
// free for your own use (host) and help lives on -?/--help.
// Try: ./custom_help -h   (sets host)   |   ./custom_help '-?'   (shows help)
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Custom help flag");

    auto& help = app.flag("-?,--help", "Show this help message");
    auto& host = app.flag("-h,--host", "Server host");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help(); return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::cout << "host = " << (host ? "set" : "unset") << "\n";
    return 0;
}
