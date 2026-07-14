#include "src/App.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    clap::App app(argv[0], "my super program");
    
    auto &verbose = app.flag("-v,--verbose", "Enable verbose output");
    auto &force = app.flag("-f,--force", "Force processing");

    auto &count = app.option<std::string>("-c,--count", "Number of lines to process").required();
    auto &names = app.multi_option<std::string>("-n,--names", "List of names").required();

    auto &input = app.positional<std::string>("input", "Input file");
    try {
        app.parse(argc, argv);
        
        if (verbose)
            std::cout << "Verbose mode enabled\n";
        if (force)
            std::cout << "Force mode enabled\n";

        if (count.is_set()) std::cout << "Count: " << count.get() << "\n";
        if (input.is_set()) std::cout << "Input file: " << input.get() << "\n";
        if (names.is_set()) {
            std::cout << "Names:\n";
            for (const auto &name : names.get())
                std::cout << "  " << name << "\n";
        }
    }
    catch (const clap::HelpRequested &e) { return 0; }
    catch (const clap::ClapException &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 84;
    }
    
    return 0;
}
