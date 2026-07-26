#pragma once

// The standard app: one argument of every kind, so most tests need no setup.
// Test files subclass it to get a feature-named GTest suite while sharing this
// exact configuration.
//
// A test needing a different setup (a required argument, a custom name, a
// registration error) declares a local `clap::App app{...}` at the top of the
// test body. It shadows the fixture's app; everything else is unchanged.

#include "clap_header.hpp"

#include <gtest/gtest.h>

#include <string>

struct StandardApp : ::testing::Test {
    clap::App app{"prog", "a test program"};
    clap::Flag& help = app.flag("-h,--help", "help");
    clap::Flag& verbose = app.flag("-v,--verbose", "verbose");
    clap::Flag& force = app.flag("-f,--force", "force");
    clap::Option<int>& count = app.option<int>("-c,--count", "count");
    clap::ValueList<std::string>& names = app.multi_option<std::string>("-n,--names", "names");
    clap::Positional<std::string>& input = app.positional<std::string>("input", "input").default_value("");
};
