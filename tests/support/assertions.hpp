#pragma once

// parse() + assert wrappers. They print useful diagnostics on failure, so
// prefer them over calling app.parse() directly.

#include "argv.hpp"
#include "clap_header.hpp"

#include <gtest/gtest.h>

// Parse and assert success (the program should carry on).
inline void expect_ok(clap::App& app, Argv& a) {
    EXPECT_TRUE(app.parse(a.argc(), a.argv())) << app.error();
}

// Parse and assert it failed with a specific ErrorKind.
inline void expect_error(clap::App& app, Argv& a, clap::ErrorKind kind) {
    ASSERT_FALSE(app.parse(a.argc(), a.argv())) << "expected an error, got none";
    EXPECT_EQ(app.error_kind(), kind);
    EXPECT_FALSE(app.error().empty());
}

// Parse and assert the help flag ended up set. Help is a plain flag, so "help
// was requested" is detected by checking the flag the caller registered.
inline void expect_help(clap::App& app, clap::Flag& help, Argv& a) {
    app.parse(a.argc(), a.argv());
    EXPECT_TRUE(help) << "expected the help flag to be set";
}
