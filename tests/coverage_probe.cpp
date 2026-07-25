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

#include <clap.hpp>

#include <string>

template class clap::Option<int>;
template class clap::Option<double>;
template class clap::Option<std::string>;

template class clap::Positional<int>;
template class clap::Positional<double>;
template class clap::Positional<std::string>;

template class clap::ValueList<int>;
template class clap::ValueList<double>;
template class clap::ValueList<std::string>;
