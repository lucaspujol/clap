// file_processor.cpp — the full feature tour: flags, typed options with
// defaults, a repeatable multi-option, and required + optional positionals.
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Process a file with assorted knobs");

    auto& verbose = app.flag("-v,--verbose", "Enable verbose output");
    auto& force   = app.flag("-f,--force", "Force processing");

    auto& count = app.option<int>("-c,--count", "Number of lines to process")
                      .default_value(10);
    auto& names = app.multi_option<std::string>("-n,--names", "Names (repeat -n)")
                      .required();

    auto& input  = app.positional<std::string>("input", "Input file");
    auto& output = app.positional<std::string>("output", "Output file")
                       .default_value("output.txt");

    try {
        app.parse(argc, argv);

        std::cout << "Verbose: " << (verbose ? "ON" : "OFF") << "\n";
        std::cout << "Force:   " << (force ? "ON" : "OFF") << "\n";
        std::cout << "Count:   " << count.get() << "\n";
        std::cout << "Names:   [";

        const auto& names_list = names.get();
        for (size_t i = 0; i < names_list.size() - 1; ++i)
            std::cout << names_list[i] << ", ";
        if (!names_list.empty())
            std::cout << names_list.back();
        
        std::cout << "]\n";
        std::cout << "Input:   " << input.get() << "\n";
        std::cout << "Output:  " << output.get() << "\n";
    } catch (const clap::HelpRequested&) {
        return 0;
    } catch (const clap::ClapException& e) {
        std::cerr << app.usage() << "\nError: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
