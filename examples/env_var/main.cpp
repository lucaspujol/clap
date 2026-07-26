#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>
#include <string>

// a server that reads its port from the command line, from the environment,
// or falls back to a built-in default:
//
//   ./env_var                        -> 8080   (the default)
//   PORT=9000 ./env_var              -> 9000   (the environment)
//   PORT=9000 ./env_var -p 3000      -> 3000   (argv wins over the environment)
//   PORT=nope ./env_var              -> error, PORT is not an int
int main(int argc, char **argv) {
    clap::App app(argv[0], "showcase of the env var fallback.");

    auto &help = app.flag("-h,--help", "show this help message");
    auto &port = app.option<int>("-p,--port", "port to listen on")
                    .from_env("PORT")           // read PORT when -p is absent
                    .default_value(8080)        // ... and this when PORT is too
                    .range(1, 65535);

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help();  return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::cout << "listening on port " << port.get() << std::endl;
}
