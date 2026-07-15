// Repeatable option: pass -t more than once to collect a list.
// Try: ./multi_option -t red -t green -t blue
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Collect repeated values");

    auto& tags = app.multi_option<std::string>("-t,--tag", "Tag (repeat -t)").required();

    try {
        app.parse(argc, argv);
    } catch (const clap::HelpRequested&) {
        return 0;
    } catch (const clap::ClapException& e) {
        std::cerr << app.usage() << "\nError: " << e.what() << "\n";
        return 1;
    }

    std::cout << tags.get().size() << " tags:\n";
    for (const auto& t : tags.get())
        std::cout << "  - " << t << "\n";
    return 0;
}
