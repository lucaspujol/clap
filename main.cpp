#include "src/App.hpp"
#include "src/ClapExceptions.hpp"
#include <iostream>
#include <string>

/*
int main(int argc, char **argv) {
    clap::App app(argv[0], "my super program");
    
    auto &verbose = app.flag(
        "-v,--verbose",
        "Enable verbose output"
    );
    auto &force = app.flag(
        "-f,--force",
        "Force processing"
    );

    auto &count = app.option<int>(
        "-c,--count",
        "Number of lines to process"
    ).default_value(10);
    auto &names = app.multi_option<std::string>(
        "-n,--names",
        "List of names"
    ).required();

    auto &input = app.positional<std::string>(
        "input",
        "Input file"
    );
    auto &output = app.positional<std::string>(
        "output",
        "Output file"
    ).default_value("output.txt");
    try {
        app.parse(argc, argv);
        
        std::cout << "Verbose: [" << (verbose ? "ON" : "OFF") << "]\n";
        std::cout << "Force:   [" << (force ? "ON" : "OFF") << "]\n";
        std::cout << "Count:   [" << count.get() << "]\n";
        std::cout << "Names:   [\n";
        for (const auto &name : names.get())
            std::cout << "  " << name << ",\n";
        std::cout << "]\n";
        std::cout << "Input:   [" << input.get() << "]\n";
        std::cout << "Output:  [" << output.get() << "]\n";
    }
    catch (const clap::HelpRequested &e) { return 0; }
    catch (const clap::ClapException &e) {
        std::cerr << app.usage() << "\n"
                  << "Error: " << e.what() << "\n";
        return 84;
    }
    
    return 0;
}
*/

int main(int argc, char **argv)
{
    clap::App app(argv[0], "calculator");

    auto &n1 = app.positional<int>("n1", "the first number of the operation");
    auto &op = app.positional<std::string>("op", "the chosen operation.");
    auto &n2 = app.positional<int>("n2", "the second number of the operation");

    try {
        app.parse(argc, argv);
        std::cout << "Result: ";
        if (op.get() == "+")
            std::cout << n1.get() + n2.get() << std::endl;
        else if (op.get() == "-")
            std::cout << n1.get() - n2.get() << std::endl;
        else if (op.get() == "*")
            std::cout << n1.get() * n2.get() << std::endl;
        else if (op.get() == "/") {
            if (n2.get() == 0)
                throw clap::InvalidValue(std::to_string(n2.get()), "second num", "non-zero");
            std::cout << n1.get() / n2.get() << std::endl;
        } else {
            throw clap::InvalidValue(op.get(), "operation", "one of +, -, *, /");
        }
    }
    catch (const clap::HelpRequested &) { return 0; }
    catch (const clap::ClapException &e) {
        std::cerr << app.usage() << "\n" << "Error: " << e.what() << std::endl;
        return 84;
    }

}