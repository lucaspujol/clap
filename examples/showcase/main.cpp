// showcase: every clap feature and every input syntax, in one program.
//
// Read examples/simplest first. It walks through App, flag, option, positional
// and parse step by step; this file assumes all of that and only covers what
// simplest leaves out.
//
// ---------------------------------------------------------------------------
// The two argument kinds simplest does not show
//   multi_option<T>    -t/--tag     a repeatable option, collects a list
//   variadic<T>()      extra        a positional that eats every leftover token
//   Both return a std::vector<T> from .get(), empty when absent.
//
// Modifiers beyond .default_value() (chainable, they return the argument)
//   .required()        parse fails if absent
//   .range(lo, hi)     reject values outside [lo, hi]
//   .choices({...})    reject values outside the set
//   .validator(fn)     reject whatever fn rejects; .range() and .choices() are
//                      sugar over it. Builtins: clap::Min, Max, FileExists,
//                      DirExists, NonexistentPath, NonEmpty
//   .from_env("KEY")   env var fallback, ranked argv > env > default > unset
//   required() and default_value() are mutually exclusive (ConfigError).
//
// Reading, beyond .get()
//   verbose.count()       how many times a flag appeared, 3 for -vvv
//   x.get_or(fallback)    like get() but never throws when nothing is set
//   app.usage()           the one-line summary, without the full help body
//   app.error_kind()      the failure as an enum, when the string is not enough
//
// Types out of the box: every arithmetic type, bool, char, std::string,
// std::filesystem::path. Anything else needs TypeName + ParseValue + operator<<
//
// ---------------------------------------------------------------------------
// Input syntax. The four spellings of one option have names, used below:
//   -p 6767        short, spaced
//   -p6767         short, attached
//   --port 6767    long, spaced
//   --port=6767    long, attached
//
// Short flags cluster; the first one that takes a value eats the rest:
//   -vv            verbose twice
//   -vp6767        verbose + port 6767
//
// Negative values MUST be attached: a spaced "-5" looks like a flag:
//   -e-5   --offset=-5     ok
//   -e -5  --offset -5     error: missing value for -e
//
// "--" ends option parsing; everything after it is positional:
//   ./showcase -n me in.txt -- --not-a-flag
//
// "/" right after the dashes parses and validates the value, then throws it
// away, leaving the argument unset (handy for scripted overrides):
//   -/v   --/port=6767
//
// ---------------------------------------------------------------------------
// Try:
//   ./showcase -h
//   ./showcase -n alice in.txt
//   ./showcase -vv -n alice -p6767 -f yaml -t red -t blue --color=off in.txt beta a b c
//   ./showcase -n alice -p 99999 in.txt        # range error
//   ./showcase -n alice -f toml in.txt         # choices error
//   ./showcase in.txt                          # missing required -n
//   SHOWCASE_TOKEN=hunter2 ./showcase -n alice in.txt
#include "clap.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

// --- a custom value type -----------------------------------------------------
// Three pieces teach clap a type: operator<<, TypeName, ParseValue.
enum class Mode { Fast, Safe, Debug };

std::ostream& operator<<(std::ostream& os, Mode m) {
    switch (m) {
        case Mode::Fast:  return os << "fast";
        case Mode::Safe:  return os << "safe";
        case Mode::Debug: return os << "debug";
    }
    assert(false && "unreachable");
    return os << "?";
}

namespace clap {
    CLAP_TYPENAME(Mode, "mode")

    template<> struct ParseValue<Mode> {
        static Mode parse(std::string_view s) {
            if (s == "fast")  return Mode::Fast;
            if (s == "safe")  return Mode::Safe;
            if (s == "debug") return Mode::Debug;
            throw clap::ParseError("expected one of: fast, safe, debug");
        }
    };
}

static const char* kind_name(clap::ErrorKind kind) {
    switch (kind) {
        case clap::ErrorKind::UnknownArgument:      return "UnknownArgument";
        case clap::ErrorKind::MissingValue:         return "MissingValue";
        case clap::ErrorKind::UnexpectedValue:      return "UnexpectedValue";
        case clap::ErrorKind::InvalidValue:         return "InvalidValue";
        case clap::ErrorKind::MissingRequiredValue: return "MissingRequiredValue";
        case clap::ErrorKind::OK:                   return "OK";
    }
    return "?";
}

int main(int argc, char** argv) {
    clap::App app(argv[0], "showcase: every clap feature in one program.");
    app.example("./showcase -n alice in.txt");
    app.example("./showcase -vv -n alice -p6767 -f yaml -t red -t blue --color=off in.txt beta a b c");
    app.example("./showcase -n alice -p 99999 in.txt", "range error");
    app.example("./showcase -n alice -f toml in.txt", "choices error");
    app.example("./showcase in.txt", "missing required -n");
    app.example("SHOWCASE_TOKEN=hunter2 ./showcase -n alice in.txt");
    app.disable_footer_wrap();
    app.footer(
        "  _________.__                  _________                       \n"
        " /   _____/|  |__   ______  _  _\\_   ___ \\_____    ______ ____  \n"
        " \\_____  \\ |  |  \\ /  _ \\ \\/ \\/ /    \\  \\/\\__  \\  /  ___// __ \\ \n"
        " /        \\|   Y  (  <_> )     /\\     \\____/ __ \\_\\___ \\\\  ___/ \n"
        "/_______  /|___|  /\\____/ \\/\\_/  \\______  (____  /____  >\\___  >\n"
        "        \\/      \\/                      \\/     \\/     \\/     \\/ \n"
    );

    auto& help    = app.flag("-h,--help", "show this help message");
    // a flag also counts its repeats, so -vvv is a verbosity level
    auto& verbose = app.flag("-v,--verbose", "repeat for more output (-vvv)");

    auto& name    = app.option<std::string>("-n,--name", "who is running this")
                        .required();

    auto& port    = app.option<int>("-p,--port", "server port")
                       .default_value(8080)
                       .range(1, 65535);

    auto& mode    = app.option<Mode>("-m,--mode", "run mode")
                       .default_value(Mode::Safe);

    auto& format  = app.option<std::string>("-f,--format", "report format")
                       .choices({"json", "yaml", "xml"})
                       .default_value("json");

    auto& ratio   = app.option<double>("-r,--ratio", "sampling ratio")
                       .default_value(0.5)
                       .range(0.0, 1.0);

    auto& offset  = app.option<int>("-e,--offset", "elevation offset, may be negative")
                       .default_value(0);

    auto& token   = app.option<std::string>("-k,--token", "API token")
                       .from_env("SHOWCASE_TOKEN");

    auto& color   = app.option<bool>("--color", "colorize output")
                       .default_value(true);

    auto& sep     = app.option<char>("--sep", "field separator")
                       .default_value(',');

    // no default and no required(): get() would throw, so read it with get_or()
    auto& output  = app.option<std::filesystem::path>("-o,--output", "output file");

    // repeatable option: -t red -t blue
    auto& tags    = app.multi_option<std::string>("-t,--tag", "tag, repeatable");

    // ordering rules between positionals: required ones must come first, and a
    // variadic must be last, so there can only be one. clap throws a
    // ConfigError at registration if these conditions arent met.
    auto& input   = app.positional<std::filesystem::path>("input", "input file");

    auto& label   = app.positional<std::string>("label", "run label")
                       .default_value("default");

    auto& extra   = app.variadic<std::string>("extra", "everything left over");

    bool ok = app.parse(argc, argv);

    if (help) { std::cout << app.help(); return 0; }
    if (!ok) {
        std::cerr << app.error();
        // error() is the text to print; error_kind() is the same failure as an
        // enum, for when the program has to branch on what went wrong.
        std::cerr << "error kind: " << kind_name(app.error_kind()) << "\n";
        return 1;
    }

    if (verbose)
        std::cout << app.usage() << "\n\n";

    std::cout << "name:      " << name.get() << "\n";
    std::cout << "verbose:   " << verbose.count() << "\n";
    std::cout << "port:      " << port.get() << "\n";
    std::cout << "mode:      " << mode.get() << "\n";
    std::cout << "format:    " << format.get() << "\n";
    std::cout << "ratio:     " << ratio.get() << "\n";
    std::cout << "offset:    " << offset.get() << "\n";
    std::cout << "token:     " << token.get_or("<none>") << "\n";
    std::cout << "color:     " << (color.get() ? "on" : "off") << "\n";
    std::cout << "sep:       " << sep.get() << "\n";
    std::cout << "input:     " << input.get() << "\n";
    std::cout << "label:     " << label.get() << "\n";
    std::cout << "output:    " << output.get_or(input.get().string() + ".out") << "\n";

    std::cout << "tags:      ";
    for (const auto& tag : tags.get())
        std::cout << tag << " ";
    std::cout << "\n";

    std::cout << "extra:     ";
    for (const auto& e : extra.get())
        std::cout << e << " ";
    std::cout << "\n";

    return 0;
}
