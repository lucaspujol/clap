#include "support/assertions.hpp"
#include "support/standard_app.hpp"
#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// create temp directory and file.
struct Validators : StandardApp {
    fs::path dir;          // exists, directory
    fs::path file;         // exists, regular file
    fs::path absent;       // does not exist
    fs::path dangling;     // a symlink whose target does not exist

    void SetUp() override {
        dir = fs::temp_directory_path() / "clap_validators_test";
        fs::remove_all(dir);
        fs::create_directories(dir);

        file = dir / "file.txt";
        std::ofstream{file} << "content";

        absent = dir / "absent.txt";
        dangling = dir / "dangling";
        fs::create_symlink(absent, dangling);
    }

    void TearDown() override { fs::remove_all(dir); }
};

// --- Stateless validators ---------------------------------------------------

TEST_F(Validators, FileExistsRejectsNonexistent) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-f", "file").validator(clap::FileExists);
    Argv a{"prog", "-f", "/this/file/does/not/exist"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, FileExistsAcceptsExisting) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-f", "file").validator(clap::FileExists);
    Argv a{"prog", "-f", file.c_str()};
    expect_ok(app, a);
}

TEST_F(Validators, DirExistsRejectsNonexistent) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-d", "directory").validator(clap::DirExists);
    Argv a{"prog", "-d", "/this/directory/does/not/exist"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, DirExistsAcceptsExisting) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-d", "directory").validator(clap::DirExists);
    Argv a{"prog", "-d", dir.c_str()};
    expect_ok(app, a);
}

TEST_F(Validators, NonExistentPathRejectsExisting) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-o", "output").validator(clap::NonexistentPath);
    Argv a{"prog", "-o", file.c_str()};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, NonExistentPathAcceptsNonExisting) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-o", "output").validator(clap::NonexistentPath);
    Argv a{"prog", "-o", "/this/file/does/not/exist"};
    expect_ok(app, a);
}

TEST_F(Validators, NonEmptyRejectsEmptyString) {
    clap::App app{"prog", "d"};
    app.option<std::string>("-s", "string").validator(clap::NonEmpty);
    Argv a{"prog", "-s", ""};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, NonEmptyAcceptsNonEmptyString) {
    clap::App app{"prog", "d"};
    app.option<std::string>("-s", "string").validator(clap::NonEmpty);
    Argv a{"prog", "-s", "hello"};
    expect_ok(app, a);
}

TEST_F(Validators, NonEmptyRejectsNonEmptyPath) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-p", "path").validator(clap::NonEmpty);
    Argv a{"prog", "-p", ""};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, NonEmptyAcceptsNonEmptyPath) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-p", "path").validator(clap::NonEmpty);
    Argv a{"prog", "-p", file.c_str()};
    expect_ok(app, a);
}

TEST_F(Validators, FileExistsRejectsDirectory) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-f", "file").validator(clap::FileExists);
    Argv a{"prog", "-f", dir.c_str()};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
    EXPECT_NE(app.error().find("is a directory"), std::string::npos);
}

TEST_F(Validators, DirExistsRejectsRegularFile) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-d", "directory").validator(clap::DirExists);
    Argv a{"prog", "-d", file.c_str()};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
    EXPECT_NE(app.error().find("is a regular file"), std::string::npos);
}

// A dangling symlink reads as absent through exists(), but it still occupies
// the name: creating there fails with EEXIST. NonexistentPath uses
// symlink_status so it rejects one.
TEST_F(Validators, NonExistentPathRejectsDanglingSymlink) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-o", "output").validator(clap::NonexistentPath);
    Argv a{"prog", "-o", dangling.c_str()};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// FileExists and DirExists take the opposite view: the link resolves to
// nothing, so there is no file and no directory there.
TEST_F(Validators, FileExistsRejectsDanglingSymlink) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-f", "file").validator(clap::FileExists);
    Argv a{"prog", "-f", dangling.c_str()};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// --- Stateful validators ----------------------------------------------------

TEST_F(Validators, RangeAcceptsInRangeValue) {
    clap::App app{"prog", "d"};
    app.option<int>("-r", "range").validator(clap::Range(1, 10));
    Argv a{"prog", "-r", "5"};
    expect_ok(app, a);
}

TEST_F(Validators, RangeRejectsOutOfRangeValue) {
    clap::App app{"prog", "d"};
    app.option<int>("-r", "range").validator(clap::Range(1, 10));
    Argv a{"prog", "-r", "100"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, MinAcceptsBiggerValue) {
    clap::App app{"prog", "d"};
    app.option<int>("-m", "Min").validator(clap::Min(5));
    Argv a{"prog", "-m", "10"};
    expect_ok(app, a);
}

TEST_F(Validators, MinRejectsSmallerValue) {
    clap::App app{"prog", "d"};
    app.option<int>("-m", "Min").validator(clap::Min(5));
    Argv a{"prog", "-m", "1"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, MaxAcceptsSmallerValue) {
    clap::App app{"prog", "d"};
    app.option<int>("-m", "Max").validator(clap::Max(5));
    Argv a{"prog", "-m", "1"};
    expect_ok(app, a);
}

TEST_F(Validators, MaxRejectsBiggerValue) {
    clap::App app{"prog", "d"};
    app.option<int>("-m", "Max").validator(clap::Max(5));
    Argv a{"prog", "-m", "10"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, ChoicesAcceptsListedValue) {
    clap::App app{"prog", "d"};
    app.option<int>("-c", "choices").validator(clap::Choices({1, 2, 3}));
    Argv a{"prog", "-c", "2"};
    expect_ok(app, a);
}

TEST_F(Validators, ChoicesRejectsUnlistedValue) {
    clap::App app{"prog", "d"};
    app.option<int>("-c", "choices").validator(clap::Choices({1, 2, 3}));
    Argv a{"prog", "-c", "4"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// --- Bounds are inclusive ---------------------------------------------------

TEST_F(Validators, RangeAcceptsBothEndpoints) {
    clap::App app{"prog", "d"};
    app.option<int>("-r", "range").validator(clap::Range(1, 10));
    Argv lo{"prog", "-r", "1"};
    expect_ok(app, lo);

    clap::App other{"prog", "d"};
    other.option<int>("-r", "range").validator(clap::Range(1, 10));
    Argv hi{"prog", "-r", "10"};
    expect_ok(other, hi);
}

TEST_F(Validators, MinAcceptsExactBound) {
    clap::App app{"prog", "d"};
    app.option<int>("-m", "min").validator(clap::Min(5));
    Argv a{"prog", "-m", "5"};
    expect_ok(app, a);
}

TEST_F(Validators, MaxAcceptsExactBound) {
    clap::App app{"prog", "d"};
    app.option<int>("-m", "max").validator(clap::Max(5));
    Argv a{"prog", "-m", "5"};
    expect_ok(app, a);
}

// --- NaN --------------------------------------------------------------------
// Every comparison against NaN is false.

TEST_F(Validators, MinRejectsNaN) {
    clap::App app{"prog", "d"};
    app.option<double>("-d", "d").validator(clap::Min(0.0));
    Argv a{"prog", "-d", "nan"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, MaxRejectsNaN) {
    clap::App app{"prog", "d"};
    app.option<double>("-d", "d").validator(clap::Max(10.0));
    Argv a{"prog", "-d", "nan"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// --- const char* overloads --------------------------------------------------
// A string literal is not storable as-is, so each builtin has an overload that
// converts to std::string. Without them these calls deduce T = const char* and
// compare pointers.

TEST_F(Validators, RangeOnStringLiteralsComparesLexicographically) {
    clap::App app{"prog", "d"};
    app.option<std::string>("-t", "tier").validator(clap::Range("a", "m"));
    Argv good{"prog", "-t", "delta"};
    expect_ok(app, good);

    clap::App other{"prog", "d"};
    other.option<std::string>("-t", "tier").validator(clap::Range("a", "m"));
    Argv bad{"prog", "-t", "zulu"};
    expect_error(other, bad, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, MinAndMaxOnStringLiterals) {
    clap::App app{"prog", "d"};
    app.option<std::string>("-t", "tier").validator(clap::Min("m"));
    Argv a{"prog", "-t", "alpha"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);

    clap::App other{"prog", "d"};
    other.option<std::string>("-t", "tier").validator(clap::Max("m"));
    Argv b{"prog", "-t", "zulu"};
    expect_error(other, b, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, ChoicesOnStringLiterals) {
    clap::App app{"prog", "d"};
    app.option<std::string>("-f", "format").validator(clap::Choices({"json", "xml"}));
    Argv good{"prog", "-f", "xml"};
    expect_ok(app, good);

    clap::App other{"prog", "d"};
    other.option<std::string>("-f", "format").validator(clap::Choices({"json", "xml"}));
    Argv bad{"prog", "-f", "yaml"};
    expect_error(other, bad, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, ChoicesFromVector) {
    clap::App app{"prog", "d"};
    std::vector<int> allowed{1, 2, 3};
    app.option<int>("-c", "choices").validator(clap::Choices(allowed));
    Argv a{"prog", "-c", "4"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// --- Custom validators ------------------------------------------------------

TEST_F(Validators, LambdaValidatorAcceptsAndRejects) {
    clap::App app{"prog", "d"};
    auto even = [](const int& v) -> std::string {
        return v % 2 == 0 ? "" : "value must be even";
    };
    app.option<int>("-n", "n").validator(even);

    Argv good{"prog", "-n", "4"};
    expect_ok(app, good);

    clap::App other{"prog", "d"};
    other.option<int>("-n", "n").validator(even);
    Argv bad{"prog", "-n", "3"};
    expect_error(other, bad, clap::ErrorKind::InvalidValue);
    // the returned string is what the user reads, verbatim.
    EXPECT_NE(other.error().find("value must be even"), std::string::npos);
}

// A lambda has neither label() nor hint(), so both `if constexpr` probes are
// skipped and the help is untouched.
TEST_F(Validators, LambdaLeavesHelpUntouched) {
    clap::App app{"prog", "d"};
    auto& opt = app.option<int>("-n", "n").validator([](const int&) { return std::string{}; });
    EXPECT_EQ(opt.type_name(), "int");
    EXPECT_TRUE(opt.hints().empty());
}

// --- Stacking ---------------------------------------------------------------

TEST_F(Validators, ValidatorsRunInRegistrationOrder) {
    clap::App app{"prog", "d"};
    app.option<int>("-n", "n")
        .validator(clap::Min(0))
        .validator([](const int&) { return std::string{"second"}; });
    Argv a{"prog", "-n-5"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
    // Min ran first and threw, so the second one never spoke.
    EXPECT_EQ(app.error().find("second"), std::string::npos);
}

TEST_F(Validators, AllValidatorsMustPass) {
    clap::App app{"prog", "d"};
    app.option<int>("-n", "n").validator(clap::Min(0)).validator(clap::Max(10));
    Argv good{"prog", "-n", "5"};
    expect_ok(app, good);

    clap::App other{"prog", "d"};
    other.option<int>("-n", "n").validator(clap::Min(0)).validator(clap::Max(10));
    Argv bad{"prog", "-n", "50"};
    expect_error(other, bad, clap::ErrorKind::InvalidValue);
}

// --- Help annotations -------------------------------------------------------
//
// label() narrows the type slot, hint() sits next to the description. Which one
// a builtin uses is part of its contract.

TEST_F(Validators, RangeAndChoicesNarrowTheTypeLabel) {
    clap::App app{"prog", "d"};
    auto& r = app.option<int>("-r", "range").validator(clap::Range(1, 10));
    EXPECT_EQ(r.type_name(), "int 1..10");
    EXPECT_TRUE(r.hints().empty());

    auto& c = app.option<std::string>("-c", "choices").validator(clap::Choices({"a", "b"}));
    EXPECT_EQ(c.type_name(), "string a|b");
}

TEST_F(Validators, StackedLabelsAppendToTheSameSlot) {
    clap::App app{"prog", "d"};
    auto& opt = app.option<int>("-n", "n")
                    .validator(clap::Range(1, 10))
                    .validator(clap::Choices({2, 4}));
    // the type is named once, then every label follows it.
    EXPECT_EQ(opt.type_name(), "int 1..10 2|4");
}

TEST_F(Validators, MinMaxAndPathValidatorsProduceHintsNotLabels) {
    clap::App app{"prog", "d"};
    auto& n = app.option<int>("-n", "n").validator(clap::Min(1)).validator(clap::Max(9));
    EXPECT_EQ(n.type_name(), "int");
    EXPECT_EQ(n.hints(), (std::vector<std::string>{">= 1", "<= 9"}));

    auto& f = app.option<std::filesystem::path>("-f", "f").validator(clap::FileExists);
    EXPECT_EQ(f.hints(), std::vector<std::string>{"must exist"});

    auto& d = app.option<std::filesystem::path>("-d", "d").validator(clap::DirExists);
    EXPECT_EQ(d.hints(), std::vector<std::string>{"must be a directory"});

    auto& o = app.option<std::filesystem::path>("-o", "o").validator(clap::NonexistentPath);
    EXPECT_EQ(o.hints(), std::vector<std::string>{"must not exist"});

    auto& s = app.option<std::string>("-s", "s").validator(clap::NonEmpty);
    EXPECT_EQ(s.hints(), std::vector<std::string>{"non-empty"});
}

// bool renders through boolalpha everywhere, labels included.
TEST_F(Validators, BoolLabelUsesBoolalpha) {
    clap::App app{"prog", "d"};
    auto& b = app.option<bool>("-b", "b").validator(clap::Choices({true}));
    EXPECT_EQ(b.type_name(), "bool true");
}

// --- Every argument kind ----------------------------------------------------

TEST_F(Validators, PositionalIsValidated) {
    clap::App app{"prog", "d"};
    app.positional<int>("n", "n").validator(clap::Min(5));
    Argv a{"prog", "1"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// A variadic runs its validator per element, so one bad token in the middle
// fails the whole parse.
TEST_F(Validators, VariadicValidatesEveryElement) {
    clap::App app{"prog", "d"};
    app.variadic<int>("ports", "ports").validator(clap::Range(1, 65535));
    Argv good{"prog", "80", "443"};
    expect_ok(app, good);

    clap::App other{"prog", "d"};
    other.variadic<int>("ports", "ports").validator(clap::Range(1, 65535));
    Argv bad{"prog", "80", "70000", "443"};
    expect_error(other, bad, clap::ErrorKind::InvalidValue);
}

TEST_F(Validators, MultiOptionValidatesEveryOccurrence) {
    clap::App app{"prog", "d"};
    app.multi_option<int>("-p,--port", "ports").validator(clap::Range(1, 65535));
    Argv a{"prog", "-p", "80", "-p", "70000"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// --- Interaction with the other value sources -------------------------------

// An env value goes through parse_value like an argv token, so validators see
// it too.
TEST_F(Validators, EnvValueIsValidated) {
    clap::App app{"prog", "d"};
    app.option<int>("-n", "n").from_env("TEST_VALIDATED").validator(clap::Min(10));
    setenv("TEST_VALIDATED", "1", 1);
    Argv a{"prog"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
    unsetenv("TEST_VALIDATED");
}

// A default is already a T and never passes through parse_value, so it is not
// validated. That is the decided behaviour (#76), not an oversight: a default
// is written by the programmer, so one that contradicts its own constraint is a
// bug to fix at the call site, not bad input to report to the CLI user.
TEST_F(Validators, DefaultValueIsNotValidated) {
    clap::App app{"prog", "d"};
    auto& n = app.option<int>("-n", "n").default_value(0).validator(clap::Min(10));
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_EQ(n.get(), 0);
}

TEST_F(Validators, DiscardedValueIsStillValidated) {
    clap::App app{"prog", "d"};
    app.option<int>("-n", "n").validator(clap::Min(10));
    Argv a{"prog", "-/n", "1"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// --- Error message ----------------------------------------------------------

// InvalidValue carries the raw token, the argument name, the (possibly
// narrowed) type label, and the validator's own message.
TEST_F(Validators, ErrorMessageCarriesTokenNameAndReason) {
    clap::App app{"prog", "d"};
    app.option<int>("-j,--jobs", "jobs").validator(clap::Range(1, 64));
    Argv a{"prog", "--jobs", "100"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);

    const std::string err = app.error();
    EXPECT_NE(err.find("100"), std::string::npos);
    EXPECT_NE(err.find("jobs"), std::string::npos);
    EXPECT_NE(err.find("1..64"), std::string::npos);
    EXPECT_NE(err.find("value out of range."), std::string::npos);
}