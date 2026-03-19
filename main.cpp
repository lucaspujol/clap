#include "App.hpp"
#include <iostream>

enum class Mode {
    FAST,
    SLOW
};

template<>
struct clap::TypeName<Mode> {
    static constexpr std::string_view value = "mode";
};

template<>
struct clap::ParseValue<Mode> {
    static Mode parse(std::string_view str) {
        if (str == "fast") return Mode::FAST;
        if (str == "slow") return Mode::SLOW;
        throw clap::ParseError("Invalid mode: " + std::string(str));
    }
};

int main(int argc, char **argv) {
    clap::App app(argv[0], "my super program");
    
    auto &verbose = app.flag("-v,--verbose", "Enable verbose output");
    auto &force = app.flag("-f,--force", "Force processing");

    auto &count = app.option<std::string>("-c,--count", "Number of lines to process").required();
    auto &mode = app.option<Mode>("--mode", "Processing mode (fast|slow). fast by default").default_value(Mode::FAST);

    auto &input = app.positional<std::string>("input", "Input file");
    try {
        app.parse(argc, argv);
        
        if (verbose)
            std::cout << "Verbose mode enabled\n";
        if (force)
            std::cout << "Force mode enabled\n";

        if (count.is_set()) std::cout << "Count: " << count.get() << "\n";
        if (input.is_set()) std::cout << "Input file: " << input.get() << "\n";
        if (mode.is_set()) std::cout << "Mode: " << (mode.get() == Mode::FAST ? "fast" : "slow") << "\n";
    }
    catch (const clap::HelpRequested &e) { return 0; }
    catch (const clap::ClapException &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 84;
    }
    
    return 0;
}
