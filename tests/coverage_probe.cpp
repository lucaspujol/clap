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

#include "support/clap_header.hpp"
#include "support/custom_type.hpp"
#include <string>

// One instantiation per DISTINCT value type: one per parsing path in
// ParseValue, not one per supported type. float behaves exactly like double
// here so instantiating it would add be pointless for actual testing, and
// add more red lines to the coverage report.
//
// Same reasoning drops uint8_t/uint16_t/uint32_t against unsigned.
//
// The list must stay in sync with the type matrix in tests/types_matrix.cpp:
// every type instantiated here is a type that should be tested in the matrix.

// list of types to instantiate
#define CLAP_PROBE_TYPES(X) \
    X(int) X(unsigned) X(char) X(double) X(bool) X(std::string) X(std::filesystem::path) X(Mode)

// explicit instantiation of every template member
#define CLAP_PROBE_INSTANTIATE(T)              \
    template class clap::Option<T>;            \
    template class clap::Positional<T>;        \
    template class clap::ValueList<T>;         \
    template class clap::detail::RangeFn<T>;   \
    template class clap::detail::ChoicesFn<T>; \
    template class clap::detail::MinFn<T>;     \
    template class clap::detail::MaxFn<T>;


// instantiate every template member with every type. ez
CLAP_PROBE_TYPES(CLAP_PROBE_INSTANTIATE)
