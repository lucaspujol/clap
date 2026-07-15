#define CLAP_IMPLEMENTATION
#include "Cli.hpp"

#include <iostream>

int main(int argc, char** argv) {
    Cli cli;
    if (auto code = cli.parse(argc, argv))
        return *code;

    std::cout << "input=" << cli.input()
              << " count=" << cli.count()
              << " verbose=" << (cli.verbose() ? "yes" : "no") << "\n";
    return 0;
}
