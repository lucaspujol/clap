#include "src/App.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    clap::App app(argv[0], "my super program");
    
    auto &verbose = app.flag(
        "-v,--verbose",
        "Enable verbose output"
    );
    auto &force = app.flag(
        "-f,--force",
        "Force processing"
    );

    auto &count = app.option<int>(
        "-c,--count",
        "Number of lines to process"
    ).required();
    auto &names = app.multi_option<std::string>(
        "-n,--names",
        "List of names"
    ).required();
    
    auto &input = app.positional<std::string>(
        "input",
        "Input file"
    );
    try {
        app.parse(argc, argv);
        
        std::cout << "Verbose: [" << (verbose ? "ON" : "OFF") << "]\n";
        std::cout << "Force:   [" << (force ? "ON" : "OFF") << "]\n";
        std::cout << "Count:   [" << count.get() << "]\n";
        std::cout << "Names:   [\n";
        for (const auto &name : names.get())
            std::cout << "  " << name << ",\n";
        std::cout << "]\n";
        std::cout << "Input:   [" << input.get() << "]\n";
    }
    catch (const clap::HelpRequested &e) { return 0; }
    catch (const clap::ClapException &e) {
        std::cerr << app.usage() << "\n"
                  << "Error: " << e.what() << "\n";
        return 84;
    }
    
    return 0;
}
