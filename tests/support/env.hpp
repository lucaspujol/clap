#pragma once

// setenv()/unsetenv() are POSIX; MSVC has _putenv_s() instead, where an empty
// value is how a variable gets removed.

#include <cstdlib>

#ifdef _WIN32
inline int setenv(const char* name, const char* value, int /*overwrite*/) {
    return _putenv_s(name, value);
}

inline int unsetenv(const char* name) {
    return _putenv_s(name, "");
}
#endif
