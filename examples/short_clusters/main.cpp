// Short-flag clustering and attached values.
// Try: ./short_clusters -vf -c10   |   -vfc 10   |   -c-5
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Short-flag clustering");

    auto& help    = app.flag("-h,--help", "Show this help message");
    auto& verbose = app.flag("-v,--verbose", "Verbose");
    auto& force   = app.flag("-f,--force", "Force");
    auto& count   = app.option<int>("-c,--count", "Count").default_value(0);

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help(); return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::cout << "verbose=" << (verbose ? 1 : 0)
              << " force=" << (force ? 1 : 0)
              << " count=" << count.get() << "\n";
    return 0;
}
