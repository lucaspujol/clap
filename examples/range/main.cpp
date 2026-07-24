#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

// imagine a game, where you must select a number of players
// per team. teams must have between 1 and 8 players
int main(int argc, char **argv) {
    clap::App app(argv[0], "showcasing the range feature");

    auto &help = app.flag("-h,--help", "show this help message");
    auto &players = app.option<int>("-p,--players", "number of players per team")
                       .range(1, 8)
                       .required();
    
    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help();  return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    int players_per_team = players.get();
    // if (players_per_team < 1 || players_per_team > 8) {
    //     std::cerr << "invalid number of players: must be between 1 & 8" << std::endl;
    //     return 1;
    // }
    std::cout << "players per team: " << players_per_team << std::endl;
    return 0;
}
