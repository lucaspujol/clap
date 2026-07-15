// greeter.cpp — smallest useful clap program: one flag, one option.
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <cctype>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Greet someone by name");

    auto& shout = app.flag("-s,--shout", "Uppercase the greeting");
    auto& name  = app.option<std::string>("-n,--name", "Who to greet")
                      .default_value("world");

    try {
        app.parse(argc, argv);
    } catch (const clap::HelpRequested&) {
        return 0;                       // -h/--help already printed help
    } catch (const clap::ClapException& e) {
        std::cerr << app.usage() << "\nError: " << e.what() << "\n";
        return 1;
    }

    std::string greeting = "Hello, " + name.get() + "!";
    if (shout)
        for (char& c : greeting)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    std::cout << greeting << "\n";
    return 0;
}
