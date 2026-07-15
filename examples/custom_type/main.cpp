// Teach clap your own type. Three pieces, all shown below:
//   1. clap::TypeName<Mode>  -> the label shown in help ("<mode>")
//   2. clap::ParseValue<Mode> -> how a string becomes a Mode
//   3. operator<<(ostream, Mode) -> lets help render the default value
// Try: ./custom_type -m safe   |   -m bogus   |   -h
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

enum class Mode { Fast, Safe, Debug };

std::ostream& operator<<(std::ostream& os, Mode m) {
    switch (m) {
        case Mode::Fast:  return os << "fast";
        case Mode::Safe:  return os << "safe";
        case Mode::Debug: return os << "debug";
    }
    return os << "?";
}

namespace clap {
    template<> struct TypeName<Mode> {
        static constexpr std::string_view value = "mode";
    };

    template<> struct ParseValue<Mode> {
        static Mode parse(std::string_view s) {
            if (s == "fast")  return Mode::Fast;
            if (s == "safe")  return Mode::Safe;
            if (s == "debug") return Mode::Debug;
            throw clap::ParseError("expected one of: fast, safe, debug");
        }
    };
}

int main(int argc, char** argv) {
    clap::App app(argv[0], "Custom type example");

    auto& mode = app.option<Mode>("-m,--mode", "Run mode: fast|safe|debug")
                     .default_value(Mode::Safe);

    try {
        app.parse(argc, argv);
    } catch (const clap::HelpRequested&) {
        return 0;
    } catch (const clap::ClapException& e) {
        std::cerr << app.usage() << "\nError: " << e.what() << "\n";
        return 1;
    }

    std::cout << "mode = " << mode.get() << "\n";
    return 0;
}
