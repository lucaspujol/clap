#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "server");
    app.no_auto_help();

    auto &help = app.flag("-h,--help", "Show this help message");

    // auto& host = app.flag("-h,--host", "server host");
    auto& port = app.option<int>("-p,--port", "port").default_value(8080);

    try {
        app.parse(argc, argv);
    } catch (const clap::ClapException& e) {
        std::cerr << app.usage() << "\nError: " << e.what() << "\n";
        return 84;
    }

    if (help) {
        std::cout << app.usage() << "\nNo auto gen for youuuu" << std::endl;
        return 0;
    }
    // std::cout << "host=" << (host ? "yes" : "no") << "\n";
    std::cout << " port=" << port.get() << "\n";
    return 0;
}
