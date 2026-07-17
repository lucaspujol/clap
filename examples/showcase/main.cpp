// showcase — a tour of every clap feature and every input syntax.
//
// Feature tour:
//   bool flag           -h/--help, -d/--debug
//   required option     -x/--width, -y/--height, -n
//   option + default    -p/--port, -c/--clients, -f/--freq
//   negative value      -e/--elevation   (attached form only — see below)
//   repeatable option   -n               (repeat it to collect a list)
//   dynamic fallback    -s/--seed        (get_or: fresh random when omitted)
//   derived fallback    -o/--output      (get_or: <input>.out when omitted)
//   positional          input
//
// Every syntax below sets the same option (-p):
//   -p 6767       short, spaced
//   -p6767        short, attached
//   --port 6767   long, spaced
//   --port=6767   long, attached
//
// Negatives MUST be attached — a spaced "-5" looks like a flag:
//   -e-5    --elevation=-5      ok
//   -e -5   --elevation -5      error (missing value for -e)
//
// Short flags cluster, and the last one may take a value:
//   -dx10     debug + width 10
//   -de-5     debug + elevation -5
//
// Full example:
//   ./showcase -p6767 -x10 -y10 -n TeamA -n TeamB -de-5 arena.conf
#include <random>
#include <string>
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int get_random_seed() {
    std::random_device rd;
    return rd();
}

int main(int argc, char **argv) {
    clap::App app(argv[0], "showcase — a tour of every clap feature and every input syntax.");

    auto &help = app.flag("-h,--help", "Show this help message");
    auto &debug = app.flag("-d,--debug", "enable debug logging");
    auto &port = app.option<int>("-p,--port", "the port of the server").default_value(8080);
    auto &width = app.option<int>("-x,--width", "the width of the map").required();
    auto &height = app.option<int>("-y,--height", "the height of the map").required();
    auto &elevation = app.option<int>("-e,--elevation", "starting elevation offset, may be negative").default_value(0);
    auto &teams = app.multi_option<std::string>("-n", "team names").required();
    auto &clients = app.option<int>("-c,--clients", "number of clients per team").default_value(10);
    auto &freq = app.option<int>("-f,--freq", "frequency of the server (events/s)").default_value(100);
    auto &seed = app.option<int>("-s,--seed", "RNG seed for reproducible games");
    auto &input = app.positional<std::string>("input", "input file");
    auto &output = app.option<std::string>("-o,--output", "output file");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help(); return 0; }
    if (!ok)  { std::cerr << app.error(); return 84; }

    std::cout << "debug:     " << (debug ? "on" : "off") << std::endl;
    std::cout << "port:      " << port.get() << std::endl;
    std::cout << "width:     " << width.get() << std::endl;
    std::cout << "height:    " << height.get() << std::endl;
    std::cout << "elevation: " << elevation.get() << std::endl;
    std::cout << "teams:     ";
    for (const auto &team : teams.get())
        std::cout << team << " ";
    std::cout << std::endl;
    std::cout << "clients:   " << clients.get() << std::endl;
    std::cout << "freq:      " << freq.get() << std::endl;
    std::cout << "seed:      " << seed.get_or(get_random_seed()) << std::endl;
    std::cout << "input:     " << input.get() << std::endl;
    std::cout << "output:    " << output.get_or(input.get() + ".out") << std::endl;

    return 0;
}
