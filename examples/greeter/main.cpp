// greeter.cpp — smallest useful clap program: one flag, one option.
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <cctype>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Greet someone by name");

    auto& help  = app.flag("-h,--help", "Show this help message");
    auto& shout = app.flag("-s,--shout", "Uppercase the greeting");
    auto& name  = app.option<std::string>("-n,--name", "Who to greet")
                      .default_value("world");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help(); return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::string greeting = "Hello, " + name.get() + "!";
    if (shout)
        for (char& c : greeting)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    std::cout << greeting << "\n";
    return 0;
}
