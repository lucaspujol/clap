// simplest: the smallest useful clap program, explained step by step.
//
// START HERE. Read this example before any other one in examples/: the others
// assume the basics below and only comment on what they add on top.
//
// Run it:
//   ./simplest world
//   ./simplest -n 3 world
//   ./simplest -n 3 world --shout
//   ./simplest -h

// first, include the header. the whole library lives in it, no other dependencies
#include "clap.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    // step 2, create an app. first argument is program name, rest is description
    clap::App app(argv[0], "greet someone");

    // step 3, register arguments. clap has four kinds: flags, options, positionals and
    // value lists (variadic & multi_option).
    //
    // A flag takes no value. It converts to bool.
    auto& help = app.flag("-h,--help", "show this help message");
    auto& shout = app.flag("-s,--shout", "shout the greeting");

    // An option takes one value: -n 3, -n3, --times 3 or --times=3.
    // .default_value() is what you get when the user does not pass it.
    auto& times = app.option<int>("-n,--times", "how many times to greet")
                      .default_value(1);

    // A positional has no dash and is matched by position. With no default it
    // is required: leaving it out is an error.
    auto& who = app.positional<std::string>("who", "who to greet");

    // step 4, parse the command line. parse() never throws: it records the first error
    // and keeps filling everything else. It returns false if there was a problem.
    bool ok = app.parse(argc, argv);

    // step 5, we check if the help flag was passed. If so, we print the help message and
    // exit. Because we check for help before checkinf for parsing errors, the user can
    // always see the help message, even if the command line is broken
    //
    // if you want to check for parse errors first, feel free to do so: clap is designed
    // to let the user keep control of the entire flow, so you get to decide what do do & when
    if (help) {
        std::cout << app.help();
        return 0;
    }

    // step 6, bail out on a parse error. app.error() is a ready-to-print
    // message: what went wrong, plus the usage line.
    if (!ok) {
        std::cerr << app.error();
        return 1;
    }

    // step 7: your logic goes here, you can now access the arguments.
    // read the values with .get(). Types are what you registered:
    // times.get() is an int, who.get() is a std::string.
    std::string greeting = "Hello, " + who.get() + "!";
    if (shout) {
        for (char& c : greeting)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    for (int i = 0; i < times.get(); ++i)
        std::cout << greeting << std::endl;

    return 0;
}
