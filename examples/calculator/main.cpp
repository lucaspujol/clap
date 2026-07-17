// calculator.cpp — positional arguments and typed parsing (int).
// Usage: ./calculator 3 + 4
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    clap::App app(argv[0], "A tiny two-operand calculator");

    auto& help = app.flag("-h,--help", "Show this help message");
    auto& n1 = app.positional<int>("n1", "first number");
    auto& op = app.positional<std::string>("op", "operation: + - * /");
    auto& n2 = app.positional<int>("n2", "second number");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help(); return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::cout << "Result: ";
    if (op.get() == "+")
        std::cout << n1.get() + n2.get() << "\n";
    else if (op.get() == "-")
        std::cout << n1.get() - n2.get() << "\n";
    else if (op.get() == "*")
        std::cout << n1.get() * n2.get() << "\n";
    else if (op.get() == "/") {
        if (n2.get() == 0)
            throw clap::InvalidValue(std::to_string(n2.get()), "n2", "non-zero");
        std::cout << n1.get() / n2.get() << "\n";
    } else {
        throw clap::InvalidValue(op.get(), "op", "one of + - * /");
    }

    return 0;
}
