#pragma once

#include "clap.hpp"

#include <optional>
#include <string>

class Cli {
public:
    // Parse argv. nullopt means "parsed OK, carry on"; a value is the exit code
    // to return right now (help was shown, or an error was printed). One line in
    // main() handles both, and the program logic stays in main().
    std::optional<int> parse(int argc, char** argv);

    bool verbose() const { return _verbose; }
    int count() const { return _count.get(); }
    const std::string& input() const { return _input.get(); }

private:
    clap::App _app{"tool", "Encapsulated clap example"};

    clap::Flag& _help = _app.flag("-h,--help", "Show this help message");
    clap::Flag& _verbose = _app.flag("-v,--verbose", "Verbose output");
    clap::Option<int>& _count = _app.option<int>("-c,--count", "Iterations")
                                    .default_value(1);
    clap::Positional<std::string>& _input =
        _app.positional<std::string>("input", "Input file");
};
