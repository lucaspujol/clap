// subcommands: git-style subcommands with two App instances.
//
// clap has no subcommand feature and is not getting one: the pattern falls out
// of what is already there. An App skips args[0] exactly as argv[0] is skipped,
// so handing the sub-app (argc - 1, argv + 1) makes the subcommand name the new
// argv[0] and the sub-app sees only the arguments meant for it. Each subcommand
// gets its own help, its own errors, its own usage line.
//
//   ./subcommands --help
//   ./subcommands commit --help
//   ./subcommands commit -m "fix the parser" --amend
//   ./subcommands add src/App.cpp tests/parsing.cpp
//   ./subcommands push --dry-run
//   ./subcommands blame            -> unknown subcommand
//
// Keep in mind that clap is not designed for such advanced use cases. It is a
// simple parser for simple command lines. This example is just to show that it is
// possible to implement subcommands with clap, but it may not be the best tool for
// complex command line interfaces: it might lack important features, and your code
// WILL become messy. If you absolutely need subcommands, consider using another 
// library that is designed for that purpose, such as CLI11 or cxxopts.
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>
#include <string>

// Each subcommand is a function taking the tail of argv. It owns its App, so
// nothing about "commit" leaks into "add". to link them together with data, we
// could use a shared struct or global variable (e.g.: files staged, commits, etc)

static int cmd_commit(int argc, char **argv) {
    clap::App app("subcommands commit", "record changes to the repository.");

    auto &help    = app.flag("-h,--help", "show this help message");
    auto &message = app.option<std::string>("-m,--message", "commit message").required();
    auto &amend   = app.flag("-a,--amend", "replace the previous commit");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help();  return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::cout << (amend ? "amending" : "committing") << ": " << message.get() << "\n";
    return 0;
}

static int cmd_add(int argc, char **argv) {
    clap::App app("subcommands add", "stage files for the next commit.");

    auto &help  = app.flag("-h,--help", "show this help message");
    auto &files = app.variadic<std::string>("files", "files to stage").required();

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help();  return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    for (const auto &file : files.get())
        std::cout << "staged " << file << "\n";
    return 0;
}

static int cmd_push(int argc, char **argv) {
    clap::App app("subcommands push", "send commits to the remote.");

    auto &help    = app.flag("-h,--help", "show this help message");
    auto &remote  = app.positional<std::string>("remote", "remote to push to").default_value("origin");
    auto &dry_run = app.flag("-n,--dry-run", "print what would be pushed, push nothing");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help();  return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::cout << (dry_run ? "would push to " : "pushing to ") << remote.get() << "\n";
    return 0;
}

// The top level app: see it like `git` command.
// it first parses the top-level arguments, then dispatches
// to the subcommand function, in our case:
//   - commit
//   - add
//   - push
int main(int argc, char **argv) {
    clap::App app(argv[0],
        "a git-shaped tool.\n"
        "\n"
        "subcommands:\n"
        "  commit    record changes to the repository\n"
        "  add       stage files for the next commit\n"
        "  push      send commits to the remote\n"
        "\n"
        "run '<subcommand> --help' for the arguments of one.");

    auto &help    = app.flag("-h,--help", "show this help message");
    auto &version = app.flag("-V,--version", "show the version and exit");

    // No subcommand: only the top-level arguments are in play.
    if (argc < 2) {
        app.parse(argc, argv);          // we parse for app.help() to be available.
        std::cout << app.help();
        return 1;
    }

    std::string command = argv[1];

    // If first argument starts with a '-', it's a top-level option, not a subcommand.
    if (!command.empty() && command.front() == '-') {
        if (!app.parse(argc, argv)) { std::cerr << app.error(); return 1; }
        if (help)    { std::cout << app.help(); return 0; }
        if (version) { std::cout << "subcommands 1.0\n"; return 0; }
        return 0;
    }

    // Everything from the subcommand name onwards belongs to the sub-app.
    if (command == "commit") return cmd_commit(argc - 1, argv + 1);
    if (command == "add")    return cmd_add(argc - 1, argv + 1);
    if (command == "push")   return cmd_push(argc - 1, argv + 1);

    std::cerr << "Error: unknown subcommand: " << command << "\n\n" << app.help();
    return 1;
}
