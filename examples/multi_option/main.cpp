// Repeatable option: pass -t more than once to collect a list.
// Try: ./multi_option -t red -t green -t blue
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Collect repeated values");

    auto& help = app.flag("-h,--help", "Show this help message");
    auto& tags = app.multi_option<std::string>("-t,--tag", "Tag (repeat -t)").required();

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help(); return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::cout << tags.get().size() << " tags:\n";
    for (const auto& t : tags.get())
        std::cout << "  - " << t << "\n";
    return 0;
}
