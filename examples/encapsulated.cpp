// encapsulated.cpp — recommended pattern: wrap clap inside your own class so
// main() is one line and every CLI concern (names, defaults, parsing, error
// handling, and the program logic) lives in one place. Nothing outside Cli
// ever sees clap's types or touches an argument value.
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>
#include <string>

class Cli {
public:
    // Runs the whole program and returns the process exit code.
    // The three parse outcomes are kept distinct on purpose:
    //   - HelpRequested : help was printed, exit cleanly, DON'T read values
    //   - ClapException : bad input, report and exit nonzero
    //   - success       : fall through to execute() and use the values
    int run(int argc, char** argv) {
        try {
            _app.parse(argc, argv);
        } catch (const clap::HelpRequested&) {
            return 0;                       // help already printed by parse()
        } catch (const clap::ClapException& e) {
            std::cerr << _app.usage() << "\nError: " << e.what() << "\n";
            return 84;
        }
        return execute();
    }

private:
    // The actual program. Reached only when every argument is populated, so
    // reading a value here can never throw MissingValue.
    int execute() {
        std::cout << "input=" << _input.get()
                  << " count=" << _count.get()
                  << " verbose=" << (_verbose ? "yes" : "no") << "\n";
        return 0;
    }

    clap::App _app{"tool", "Encapsulated clap example"};

    clap::Flag& _verbose = _app.flag("-v,--verbose", "Verbose output");
    clap::Option<int>& _count = _app.option<int>("-c,--count", "Iterations").default_value(1);
    clap::Positional<std::string>& _input = _app.positional<std::string>("input", "Input file");
};

int main(int argc, char** argv) {
    Cli cli;
    return cli.run(argc, argv);
}
