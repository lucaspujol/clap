// separator.cpp — shows the `--` separator: everything after it is a
// positional, even tokens that start with a dash. Try:
//   ./separator a.txt b.txt c.txt
//   ./separator --log -- -weird.txt --also-weird.txt normal.txt
//   ./separator -- -- b.txt          (second `--` is just a positional)
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Demo of the -- positional separator");

    auto& help = app.flag("-h,--help", "Show this help message");
    auto& log  = app.flag("-l,--log", "Enable logging");

    auto& first  = app.positional<std::string>("first", "First file");
    auto& second = app.positional<std::string>("second", "Second file");
    auto& third  = app.positional<std::string>("third", "Third file");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help(); return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::cout << "Log:    " << (log ? "ON" : "OFF") << "\n";
    std::cout << "First:  " << first.get() << "\n";
    std::cout << "Second: " << second.get() << "\n";
    std::cout << "Third:  " << third.get() << "\n";
    return 0;
}
