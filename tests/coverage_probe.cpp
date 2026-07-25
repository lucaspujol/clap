// Coverage probe - built only when CLAP_COVERAGE=ON.
//
// Two kinds of code are invisible to gcov and so score as "covered" when they
// are not tested at all:
//
//   1. Inline members that are never called. The compiler never emits them, so
//      no line ever appears in the report. Fixed by -fkeep-inline-functions,
//      applied to this file only (on the test target it also forces emission of
//      unused gtest inlines, which fail to link against a prebuilt libgtest).
//
//   2. Template members that are never instantiated. Fixed by the explicit
//      instantiations below: they force every member of each listed type to be
//      compiled, so untested ones show up red.
//
// This file executes nothing.
// CLAP_IMPLEMENTATION is deliberately NOT defined here.

#include "support/clap_header.hpp"
#include <string>

// One instantiation per type clap::TypeName declares a label for — that is the
// set the library claims to support, so a red line here is a supported type
// with no test behind it. Instantiating anything else would only invent gaps.

#define CLAP_PROBE_TYPES(X) \
    X(int) X(float) X(double) X(bool) X(std::string) X(std::filesystem::path)

#define CLAP_PROBE_INSTANTIATE(T)     \
    template class clap::Option<T>;   \
    template class clap::Positional<T>; \
    template class clap::ValueList<T>;

CLAP_PROBE_TYPES(CLAP_PROBE_INSTANTIATE)
