// Short-flag clustering and attached values.
// Try: ./short_clusters -vf -c10   |   -vfc 10   |   -c-5
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Short-flag clustering");

    auto& verbose = app.flag("-v,--verbose", "Verbose");
    auto& force   = app.flag("-f,--force", "Force");
    auto& count   = app.option<int>("-c,--count", "Count").default_value(0);

    try {
        app.parse(argc, argv);
    } catch (const clap::HelpRequested&) {
        return 0;
    } catch (const clap::ClapException& e) {
        std::cerr << app.usage() << "\nError: " << e.what() << "\n";
        return 1;
    }

    std::cout << "verbose=" << (verbose ? 1 : 0)
              << " force=" << (force ? 1 : 0)
              << " count=" << count.get() << "\n";
    return 0;
}
