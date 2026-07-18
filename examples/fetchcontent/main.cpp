#define CLAP_IMPLEMENTATION   // exactly ONE .cpp file gets this line
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "fetchcontent demo");

    auto& help  = app.flag("-h,--help", "show this help");
    auto& name  = app.option<std::string>("-n,--name", "who to greet")
                      .default_value("world");
    auto& count = app.option<int>("-c,--count", "how many times")
                      .default_value(1);

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help();  return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    for (int i = 0; i < count.get(); ++i)
        std::cout << "Hello, " << name.get() << "!\n";
    return 0;
}
