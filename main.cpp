#include "App.hpp"
#include <iostream>

int main(int argc, char **argv) {
    clap::App app(argv[0], "my super program");
    
    auto &verbose = app.flag("-v,--verbose", "Enable verbose output");
    auto &count = 
        app.option<int>("-c,--count", "Number of times to do something")
        .required();
    app.parse(argc, argv);
    
    if (verbose)
        std::cout << "Verbose mode enabled\n";

    std::cout << "Count: " << count.get() << "\n";

    return 0;
}
