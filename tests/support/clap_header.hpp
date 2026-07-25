#pragma once

// Every test TU includes the shipped single header through here. The
// implementation is compiled exactly once, in tests/clap_impl.cpp, which is the
// only place CLAP_IMPLEMENTATION may be defined.

#include "clap.hpp"
