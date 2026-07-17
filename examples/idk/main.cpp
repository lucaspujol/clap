#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char **argv) {
    clap::App app(argv[0], "testing stuff");

    auto &help = app.flag("-h,--help", "Show this help message");
    auto &Bool = app.flag("-b,--bool", "a bool");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help(); return 0; }
    if (!ok)  { std::cerr << app.error(); return 84; }

    std::cout << "bool=" << (Bool ? "true" : "false") << std::endl;
    return 0;
}
