// typed driven tests. Uses gtests `TYPED_TEST` instead of
// the regular `TEST_F`.

#include "support/assertions.hpp"
#include "support/custom_type.hpp"
#include "support/env.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

// Per-type sample values. `bad` and `outside` are null when the type has no
// such token: every string parses, and bool has nothing outside false..true.
template<typename T>
struct Sample;

template<>
struct Sample<int> {
    static constexpr const char* name = "int";
    static constexpr const char* label = "int";
    static constexpr const char* tok_a = "10";
    static constexpr const char* tok_b = "20";
    static constexpr const char* text_a = "10";
    static constexpr const char* bad = "abc";
    static constexpr const char* outside = "1000";
    static int a() { return 10; }
    static int b() { return 20; }
    static int lo() { return 0; }
    static int hi() { return 100; }
};

template<>
struct Sample<unsigned> {
    static constexpr const char* name = "unsigned";
    static constexpr const char* label = "uint";
    static constexpr const char* tok_a = "10";
    static constexpr const char* tok_b = "20";
    static constexpr const char* text_a = "10";
    static constexpr const char* bad = "-5";
    static constexpr const char* outside = "1000";
    static unsigned a() { return 10u; }
    static unsigned b() { return 20u; }
    static unsigned lo() { return 0u; }
    static unsigned hi() { return 100u; }
};

// char is its own parsing path: one character, never a number.
template<>
struct Sample<char> {
    static constexpr const char* name = "char";
    static constexpr const char* label = "char";
    static constexpr const char* tok_a = "a";
    static constexpr const char* tok_b = "b";
    static constexpr const char* text_a = "a";
    static constexpr const char* bad = "ab";
    static constexpr const char* outside = "z";
    static char a() { return 'a'; }
    static char b() { return 'b'; }
    static char lo() { return 'a'; }
    static char hi() { return 'c'; }
};

template<>
struct Sample<double> {
    static constexpr const char* name = "double";
    static constexpr const char* label = "double";
    static constexpr const char* tok_a = "1.5";
    static constexpr const char* tok_b = "2.5";
    static constexpr const char* text_a = "1.5";
    static constexpr const char* bad = "zz";
    static constexpr const char* outside = "99.5";
    static double a() { return 1.5; }
    static double b() { return 2.5; }
    static double lo() { return 0.0; }
    static double hi() { return 10.0; }
};

template<>
struct Sample<bool> {
    static constexpr const char* name = "bool";
    static constexpr const char* label = "bool";
    static constexpr const char* tok_a = "true";
    static constexpr const char* tok_b = "false";
    static constexpr const char* text_a = "true";
    static constexpr const char* bad = "maybe";
    static constexpr const char* outside = nullptr;
    static bool a() { return true; }
    static bool b() { return false; }
    static bool lo() { return false; }
    static bool hi() { return true; }
};

template<>
struct Sample<std::string> {
    static constexpr const char* name = "string";
    static constexpr const char* label = "string";
    static constexpr const char* tok_a = "alpha";
    static constexpr const char* tok_b = "beta";
    static constexpr const char* text_a = "alpha";
    static constexpr const char* bad = nullptr;
    static constexpr const char* outside = "zzzz";
    static std::string a() { return "alpha"; }
    static std::string b() { return "beta"; }
    static std::string lo() { return "a"; }
    static std::string hi() { return "zzz"; }
};

template<>
struct Sample<std::filesystem::path> {
    // operator<<(ostream, path) quotes, so the rendered default is quoted too.
    static constexpr const char* name = "path";
    static constexpr const char* label = "path";
    static constexpr const char* tok_a = "dir/a.txt";
    static constexpr const char* tok_b = "dir/b.txt";
    static constexpr const char* text_a = "\"dir/a.txt\"";
    static constexpr const char* bad = nullptr;
    static constexpr const char* outside = "zzzz";
    static std::filesystem::path a() { return "dir/a.txt"; }
    static std::filesystem::path b() { return "dir/b.txt"; }
    static std::filesystem::path lo() { return "a"; }
    static std::filesystem::path hi() { return "zzz"; }
};

template<>
struct Sample<Mode> {
    static constexpr const char* name = "custom";
    static constexpr const char* label = "mode";
    static constexpr const char* tok_a = "safe";
    static constexpr const char* tok_b = "fast";
    static constexpr const char* text_a = "safe";
    static constexpr const char* bad = "bogus";
    static constexpr const char* outside = "debug";
    static Mode a() { return Mode::Safe; }
    static Mode b() { return Mode::Fast; }
    static Mode lo() { return Mode::Fast; }
    static Mode hi() { return Mode::Safe; }
};

template<typename T>
struct ValueTypes : ::testing::Test {
    clap::App app{"prog", "a test program"};
};

struct TypeNames {
    template<typename T>
    static std::string GetName(int) { return Sample<T>::name; }
};

typedef testing::Types<
    int, unsigned, char, double,
    bool, std::string,
    std::filesystem::path,
    Mode>
MatrixTypes;

TYPED_TEST_SUITE(ValueTypes, MatrixTypes, TypeNames);

TYPED_TEST(ValueTypes, OptionParsesAndReports) {
    using S = Sample<TypeParam>;
    auto& opt = this->app.template option<TypeParam>("-x,--xx", "x");
    Argv a{"prog", "-x", S::tok_a};
    expect_ok(this->app, a);

    EXPECT_EQ(opt.get(), S::a());
    EXPECT_TRUE(opt.is_set());
    EXPECT_TRUE(opt.takes_value());
    EXPECT_EQ(opt.type_name(), S::label);
    EXPECT_EQ(opt.env_key(), "");
    EXPECT_EQ(opt.default_str(), "");
    EXPECT_EQ(opt.get_or(S::b()), S::a());
}

TYPED_TEST(ValueTypes, OptionEqualsFormAndDefault) {
    using S = Sample<TypeParam>;
    auto& opt = this->app.template option<TypeParam>("-x,--xx", "x")
                    .default_value(S::a());
    EXPECT_EQ(opt.default_str(), S::text_a);

    Argv a{"prog", std::string("--xx=") + S::tok_b};
    expect_ok(this->app, a);
    EXPECT_EQ(opt.get(), S::b());
}

TYPED_TEST(ValueTypes, OptionFallsBackToDefaultThenFallback) {
    using S = Sample<TypeParam>;
    auto& defaulted = this->app.template option<TypeParam>("-x,--xx", "x")
                          .default_value(S::a());
    auto& bare = this->app.template option<TypeParam>("-y,--yy", "y");

    Argv a{"prog"};
    expect_ok(this->app, a);

    EXPECT_FALSE(defaulted.is_set());
    EXPECT_EQ(defaulted.get(), S::a());
    EXPECT_EQ(defaulted.get_or(S::b()), S::a());
    EXPECT_THROW(bare.get(), clap::MissingValue);
    EXPECT_EQ(bare.get_or(S::b()), S::b());
}

TYPED_TEST(ValueTypes, OptionRequiredExcludesDefault) {
    using S = Sample<TypeParam>;
    auto& opt = this->app.template option<TypeParam>("-x,--xx", "x").required();
    EXPECT_THROW(opt.default_value(S::a()), clap::ConfigError);

    auto& other = this->app.template option<TypeParam>("-y,--yy", "y")
                      .default_value(S::a());
    EXPECT_THROW(other.required(), clap::ConfigError);
}

TYPED_TEST(ValueTypes, MultiOptionRequiredExcludesDefault) {
    using S = Sample<TypeParam>;
    auto& opt = this->app.template multi_option<TypeParam>("-x,--xx", "x").required();
    EXPECT_THROW(opt.default_value({S::a()}), clap::ConfigError);

    auto& other = this->app.template multi_option<TypeParam>("-y,--yy", "y")
                      .default_value({S::a()});
    EXPECT_THROW(other.required(), clap::ConfigError);
}

TYPED_TEST(ValueTypes, MultiOptionDefaultListIsUsedWhenAbsent) {
    using S = Sample<TypeParam>;
    auto& opt = this->app.template multi_option<TypeParam>("-x,--xx", "x")
                    .default_value({S::a(), S::b()});
    Argv a{"prog"};
    expect_ok(this->app, a);
    EXPECT_FALSE(opt.is_set());
    ASSERT_EQ(opt.get().size(), 2u);
    EXPECT_EQ(opt.get()[0], S::a());
    EXPECT_EQ(opt.get()[1], S::b());
}

TYPED_TEST(ValueTypes, OptionRequiredMissingIsReported) {
    this->app.template option<TypeParam>("-x,--xx", "x").required();
    Argv a{"prog"};
    expect_error(this->app, a, clap::ErrorKind::MissingRequiredValue);
}

TYPED_TEST(ValueTypes, OptionReadsFromEnv) {
    using S = Sample<TypeParam>;
    const std::string key = std::string("CLAP_MATRIX_") + S::name;
    auto& opt = this->app.template option<TypeParam>("-x,--xx", "x").from_env(key);
    EXPECT_EQ(opt.env_key(), key);

    setenv(key.c_str(), S::tok_a, 1);
    Argv a{"prog"};
    expect_ok(this->app, a);
    EXPECT_EQ(opt.get(), S::a());
    unsetenv(key.c_str());
}

TYPED_TEST(ValueTypes, PositionalParsesAndDefaults) {
    using S = Sample<TypeParam>;
    auto& required = this->app.template positional<TypeParam>("first", "first");
    auto& optional = this->app.template positional<TypeParam>("second", "second")
                         .default_value(S::b());

    EXPECT_TRUE(required.is_required());
    EXPECT_TRUE(required.takes_value());
    EXPECT_FALSE(optional.is_required());
    EXPECT_FALSE(optional.default_str().empty());

    Argv a{"prog", S::tok_a};
    expect_ok(this->app, a);

    EXPECT_TRUE(required.is_set());
    EXPECT_EQ(required.get(), S::a());
    EXPECT_EQ(required.type_name(), S::label);
    EXPECT_EQ(required.default_str(), "");
    EXPECT_FALSE(optional.is_set());
    EXPECT_EQ(optional.get(), S::b());
}

TYPED_TEST(ValueTypes, PositionalUnsetThrows) {
    auto& pos = this->app.template positional<TypeParam>("first", "first");
    EXPECT_THROW(pos.get(), clap::MissingValue);
}

TYPED_TEST(ValueTypes, MultiOptionCollects) {
    using S = Sample<TypeParam>;
    auto& list = this->app.template multi_option<TypeParam>("-x,--xx", "x");
    EXPECT_TRUE(list.is_multi());
    EXPECT_TRUE(list.takes_value());
    EXPECT_EQ(list.type_name(), S::label);

    Argv a{"prog", "-x", S::tok_a, "--xx", S::tok_b};
    expect_ok(this->app, a);

    EXPECT_TRUE(list.is_set());
    EXPECT_EQ(list.get(), (std::vector<TypeParam>{S::a(), S::b()}));
}

TYPED_TEST(ValueTypes, MultiOptionEmptyUnlessRequired) {
    auto& list = this->app.template multi_option<TypeParam>("-x,--xx", "x");
    Argv a{"prog"};
    expect_ok(this->app, a);
    EXPECT_FALSE(list.is_set());
    EXPECT_TRUE(list.get().empty());

    clap::App strict{"prog", "d"};
    auto& required = strict.multi_option<TypeParam>("-x,--xx", "x").required();
    Argv b{"prog"};
    expect_error(strict, b, clap::ErrorKind::MissingRequiredValue);
    EXPECT_THROW(required.get(), clap::MissingValue);
}

TYPED_TEST(ValueTypes, VariadicCollectsRemainingTokens) {
    using S = Sample<TypeParam>;
    auto& rest = this->app.template variadic<TypeParam>("rest", "rest");
    Argv a{"prog", S::tok_a, S::tok_b};
    expect_ok(this->app, a);
    EXPECT_EQ(rest.get(), (std::vector<TypeParam>{S::a(), S::b()}));
}

TYPED_TEST(ValueTypes, ChoicesAcceptsListedAndRejectsOther) {
    using S = Sample<TypeParam>;
    auto& opt = this->app.template option<TypeParam>("-x,--xx", "x")
                    .choices({S::a()});
    // choices list replaces the type in the help label.
    // ex: <int> becomes <10> if the only choice is 10.
    EXPECT_EQ(opt.type_name(), S::text_a);

    Argv good{"prog", "-x", S::tok_a};
    expect_ok(this->app, good);
    EXPECT_EQ(opt.get(), S::a());

    clap::App other{"prog", "d"};
    other.option<TypeParam>("-x,--xx", "x").choices({S::a()});
    Argv bad{"prog", "-x", S::tok_b};
    expect_error(other, bad, clap::ErrorKind::InvalidValue);
}

TYPED_TEST(ValueTypes, RangeAcceptsInsideAndRejectsOutside) {
    using S = Sample<TypeParam>;
    auto& opt = this->app.template option<TypeParam>("-x,--xx", "x")
                    .range(S::lo(), S::hi());
    // ranges show <int 1..10> instead of <int> in the help label.
    EXPECT_NE(opt.type_name().find(S::label), std::string_view::npos);

    Argv good{"prog", "-x", S::tok_a};
    expect_ok(this->app, good);
    EXPECT_EQ(opt.get(), S::a());

    if (S::outside == nullptr)
        return;  // bool: nothing lies outside false..true
    clap::App other{"prog", "d"};
    other.option<TypeParam>("-x,--xx", "x").range(S::lo(), S::hi());
    Argv bad{"prog", "-x", S::outside};
    expect_error(other, bad, clap::ErrorKind::InvalidValue);
}

// `if constexpr`, not a plain `if`: a runtime guard still compiles the body for
// string & path, and passing their null `bad` to setenv() warns (-Wnonnull).

TYPED_TEST(ValueTypes, BadValueIsReported) {
    using S = Sample<TypeParam>;
    if constexpr (S::bad != nullptr) {  // string & path take anything as value
        this->app.template option<TypeParam>("-x,--xx", "x");
        Argv a{"prog", std::string("--xx=") + S::bad};   // eq form to make sure its parsed
        expect_error(this->app, a, clap::ErrorKind::InvalidValue);
    }
}

TYPED_TEST(ValueTypes, BadEnvValueIsReported) {
    using S = Sample<TypeParam>;
    if constexpr (S::bad != nullptr) {
        const std::string key = std::string("CLAP_MATRIX_BAD_") + S::name;
        this->app.template option<TypeParam>("-x,--xx", "x").from_env(key);
        setenv(key.c_str(), S::bad, 1);
        Argv a{"prog"};
        expect_error(this->app, a, clap::ErrorKind::InvalidValue);
        unsetenv(key.c_str());
    }
}

TYPED_TEST(ValueTypes, HelpRendersLabelAndDefault) {
    using S = Sample<TypeParam>;
    this->app.template option<TypeParam>("-x,--xx", "x").default_value(S::a());
    const std::string help = this->app.help();
    EXPECT_NE(help.find(S::label), std::string::npos);
    EXPECT_NE(help.find(S::text_a), std::string::npos);
}
