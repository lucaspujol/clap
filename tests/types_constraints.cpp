// Value constraints applied after a successful conversion: .choices() and
// .range(). Both run on every argument kind, so the interesting cases are the
// ones that check they still fire on positionals and per-element on lists.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

struct RangeChoices : StandardApp {};

TEST_F(RangeChoices, ChoicesAcceptsListedValue) {
    clap::App app{"prog", "d"};
    auto& fmt = app.option<std::string>("-f,--format", "format")
                    .choices({"json", "xml", "yaml"});
    Argv a{"prog", "-f", "xml"};
    expect_ok(app, a);
    EXPECT_EQ(fmt.get(), "xml");
}

TEST_F(RangeChoices, ChoicesRejectsUnlistedValue) {
    clap::App app{"prog", "d"};
    app.option<std::string>("-f,--format", "format").choices({"json", "xml", "yaml"});
    Argv a{"prog", "-f", "jsn"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(RangeChoices, ChoicesHelpListsAlternatives) {
    clap::App app{"prog", "d"};
    app.option<std::string>("-f,--format", "format").choices({"json", "xml", "yaml"});
    EXPECT_NE(app.help().find("<string json|xml|yaml>"), std::string::npos);
}

TEST_F(RangeChoices, ChoicesOnPositionalRejects) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("mode", "mode").choices({"fast", "safe"});
    Argv a{"prog", "turbo"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(RangeChoices, RangeAcceptsValueInside) {
    clap::App app{"prog", "d"};
    auto& jobs = app.option<int>("-j,--jobs", "jobs").range(1, 64);
    Argv a{"prog", "-j", "8"};
    expect_ok(app, a);
    EXPECT_EQ(jobs.get(), 8);
}

TEST_F(RangeChoices, RangeAcceptsInclusiveBound) {
    clap::App app{"prog", "d"};
    auto& jobs = app.option<int>("-j,--jobs", "jobs").range(1, 64);
    Argv a{"prog", "-j", "64"};
    expect_ok(app, a);
    EXPECT_EQ(jobs.get(), 64);
}

TEST_F(RangeChoices, RangeRejectsValueAbove) {
    clap::App app{"prog", "d"};
    app.option<int>("-j,--jobs", "jobs").range(1, 64);
    Argv a{"prog", "-j", "7000"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(RangeChoices, RangeRejectsValueBelow) {
    clap::App app{"prog", "d"};
    app.option<int>("-j,--jobs", "jobs").range(1, 64);
    Argv a{"prog", "-j", "0"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// range() leans on operator<=, so on strings it compares lexicographically.
TEST_F(RangeChoices, RangeWorksLexicographicallyOnStrings) {
    clap::App app{"prog", "d"};
    auto& tier = app.option<std::string>("-t,--tier", "tier").range("a", "m");
    Argv a{"prog", "-t", "gold"};
    expect_ok(app, a);
    EXPECT_EQ(tier.get(), "gold");
}

TEST_F(RangeChoices, RangeRejectsStringOutsideLexRange) {
    clap::App app{"prog", "d"};
    app.option<std::string>("-t,--tier", "tier").range("a", "m");
    Argv a{"prog", "-t", "zeta"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// On a list the constraint runs per element, not on the collection.
TEST_F(RangeChoices, RangeCheckedPerElementOnVariadic) {
    clap::App app{"prog", "d"};
    app.variadic<int>("ports", "ports").range(1, 65535);
    Argv a{"prog", "22", "8080", "70000"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// NaN compares false against everything, so a range test written as
// "v < lo || hi < v" would let it through. The bounds are checked positively.
TEST_F(RangeChoices, RangeRejectsNaN) {
    clap::App app{"prog", "d"};
    app.option<double>("-d", "d").range(0.0, 10.0);
    Argv a{"prog", "-d", "nan"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(RangeChoices, RangeRejectsInfinity) {
    clap::App app{"prog", "d"};
    app.option<double>("-d", "d").range(0.0, 10.0);
    Argv a{"prog", "-d", "inf"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(RangeChoices, RangeAcceptsBounds) {
    clap::App app{"prog", "d"};
    auto& d = app.option<double>("-d", "d").range(0.0, 10.0);
    for (const char* token : {"0", "10", "4.5"}) {
        Argv a{"prog", "-d", token};
        expect_ok(app, a);
        EXPECT_TRUE(d.is_set()) << "token: " << token;
    }
}
