#define CLAP_IMPLEMENTATION
#include "clap.hpp"

int main(int argc, char **argv) {
    clap::App app(argv[0], "zappy_server - end of year project epitech");

    auto &port = app.option<int>("-p,--port", "the port of the server").required();
    auto &width = app.option<int>("-x,--width", "the width of the map").required();
    auto &height = app.option<int>("-y,--height", "the height of the map").required();
    auto &teams = app.multi_option<std::string>("-n", "team names").required();
    auto &clients = app.option<int>("-c,--clients", "number of clients per team").default_value(10);
    auto &freq = app.option<int>("-f,--freq", "frequency of the server (events/s)").default_value(100);
    auto &seed = app.option<int>("-s,--seed", "RNG seed for reproducible games").default_value(8787);       // shoulda be random but i cant be asked

    
    try {
        app.parse(argc, argv);
        std::cout << "port:    " << port.get() << std::endl;
        std::cout << "width:   " << width.get() << std::endl;
        std::cout << "height:  " << height.get() << std::endl;
        std::cout << "teams:   ";
        for (const auto &team : teams.get())
            std::cout << team << " ";
        std::cout << std::endl;
        std::cout << "clients: " << clients.get() << std::endl;
        std::cout << "freq:    " << freq.get() << std::endl;
        std::cout << "seed:    " << seed.get() << std::endl;
    }
    catch (const clap::HelpRequested&) { return 0; }
    catch (const clap::ClapException &e) {
        std::cerr << "Error: " << e.what() << "\n" << app.usage() << std::endl;
        return 84;
    }
    return 0;
}
