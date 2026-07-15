// Catch specific clap exceptions to react differently per error.
// Try: ./error_handling --nope   |   -c abc   |   -c
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Distinguish error types");

    auto& count = app.option<int>("-c,--count", "A number").default_value(0);

    try {
        app.parse(argc, argv);
    } catch (const clap::HelpRequested&) {
        return 0;
    } catch (const clap::UnknownArgument& e) {
        std::cerr << "unknown argument: " << e.what() << "\n";
        return 2;
    } catch (const clap::MissingValue& e) {
        std::cerr << "missing value: " << e.what() << "\n";
        return 3;
    } catch (const clap::InvalidValue& e) {
        std::cerr << "invalid value: " << e.what() << "\n";
        return 4;
    } catch (const clap::ClapException& e) {
        std::cerr << "other error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "count=" << count.get() << "\n";
    return 0;
}
