// The one translation unit that compiles clap's implementation. Every other
// test file includes the header through support/clap_header.hpp, which does not
// define CLAP_IMPLEMENTATION — defining it twice would break the link.
//
// Tests deliberately build against the shipped single header rather than the
// sources in src/, so the amalgamation itself is what gets exercised.

#define CLAP_IMPLEMENTATION
#include "clap.hpp"
