// testing stuff idk
#include <iostream>
#include <string>

#define CLAP_IMPLEMENTATION
#include "clap.hpp"

int main(int argc, char **argv) {
    clap::App app(argv[0], "testing stuff idk");

    auto &help = app.flag("-h,--help", "show help message");
    auto &count = app.option<int>("-c,--count", "count")
                                    .required();
    auto &conf_file = app.positional<std::string>(
        "conf_file", 
        "configuration file to launch the app"
    );
    auto &output = app.option<std::string>(
        "-o", 
        "output file (default <input>.out)"
    );

    bool ok = app.parse(argc, argv);
    if (help) {
        std::cout << app.help();
        return 0;
    }
    if (!ok) {
        std::cerr << app.error();
        return 67;
    }
    std::cout << "count is: " << count.get() << std::endl;
    std::cout << "config file is: " << conf_file.get() << std::endl;
    std::cout << "output file is: " << output.get_or(conf_file.get() + ".out") << std::endl;
}
