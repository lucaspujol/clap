// struct_args — defining your flags as a struct, JAI-style.
// thanks tsoding for your recent video. clap using clap in
// the way you showed in the video. no api changes, just 
// different way to abstract argument parsing
//
//   ./struct_args -p6767 -s10 -n TeamA -n TeamB -d arena.conf
#include <string>
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

// ---- Pattern A: the schema struct ------------------------------------------
struct Args {
    clap::App app{"struct_args", "define your flags as a struct, JAI-style."};

    clap::Flag& debug = app.flag("-d,--debug", "enable debug logging");
    clap::Option<int>& port = app.option<int>("-p,--port", "server port").default_value(8080);
    clap::Option<int>& size = app.option<int>("-s,--size", "map size").required();
    clap::Option<int>& elevation = app.option<int>("-e,--elevation", "elevation offset").default_value(0);
    clap::MultiOption<std::string>& teams = app.multi_option<std::string>("-n", "team names").required();
    clap::Positional<std::string>& input = app.positional<std::string>("input", "input file");
    clap::Option<std::string>& output = app.option<std::string>("-o,--output", "the output (default: <input>.out)");
    clap::Flag& help = app.flag("-h,--help", "show this help message");

    bool ok;
    Args(int argc, char** argv) { ok = app.parse(argc, argv); }
};

// ---- Pattern B: snapshot to plain values -----------------------------------
// this is just to have less verbose when getting the values
struct Config {
    bool debug;
    int port;
    int size;
    int elevation;
    std::vector<std::string> teams;
    std::string input;
    std::string output;
};

// gives a snapshot: rerun if values were subject to change
// use get() / get_or() as you want, 
Config snapshot(const Args& a) {
    return Config{
        .debug     = a.debug,
        .port      = a.port.get(),
        .size      = a.size.get(),
        .elevation = a.elevation.get(),
        .teams     = a.teams.get(),
        .input     = a.input.get(),
        .output    = a.output.get_or(a.input.get() + ".out"),
    };
}

int main(int argc, char** argv) {
    Args args(argc, argv);
    if (args.help) { std::cout << args.app.help(); return 0; }
    if (!args.ok)  { std::cerr << args.app.error(); return 84; }

    // Plain-value struct: args.port.get() becomes cfg.port.
    Config cfg = snapshot(args);

    std::cout << "debug:     " << (cfg.debug ? "on" : "off") << "\n";
    std::cout << "port:      " << cfg.port << "\n";
    std::cout << "size:      " << cfg.size << "\n";
    std::cout << "elevation: " << cfg.elevation << "\n";
    std::cout << "teams:     ";
    for (const auto& team : cfg.teams)
        std::cout << team << " ";
    std::cout << "\n";
    std::cout << "input:     " << cfg.input << "\n";
    std::cout << "output:    " << cfg.output << "\n";
    return 0;
}
