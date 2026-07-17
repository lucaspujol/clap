// Wrap the App in your own class so all the parse/help/error plumbing lives in
// Cli, and argument handling in main() collapses to a single guard line. The
// program itself stays right here in main(), reading values off the Cli.
#define CLAP_IMPLEMENTATION
#include "Cli.hpp"

#include <iostream>

int main(int argc, char** argv) {
    Cli cli;
    if (auto exit_code = cli.parse(argc, argv))
        return *exit_code;

    std::cout << "input=" << cli.input() << std::endl;
    std::cout << " count=" << cli.count() << std::endl;
    std::cout << " verbose=" << (cli.verbose() ? "yes" : "no") << std::endl;
    return 0;
}
