#pragma once

#include "clap.hpp"

#include <optional>
#include <string>

class Cli {
public:
    // nullopt: parsed OK, proceed. value: exit now with this code.
    std::optional<int> parse(int argc, char** argv);

    bool verbose() const { return _verbose; }
    int count() const { return _count.get(); }
    const std::string& input() const { return _input.get(); }

private:
    clap::App _app{"tool", "Encapsulated clap example"};

    clap::Flag& _verbose = _app.flag("-v,--verbose", "Verbose output");
    clap::Option<int>& _count = _app.option<int>("-c,--count", "Iterations")
                                    .default_value(1);
    clap::Positional<std::string>& _input =
        _app.positional<std::string>("input", "Input file");
};
