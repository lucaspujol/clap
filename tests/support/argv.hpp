#pragma once

// Builds a mutable char** argv (as main() receives) from a list of strings.

#include <initializer_list>
#include <string>
#include <vector>

struct Argv {
    std::vector<std::string> storage;
    std::vector<char*> ptrs;

    Argv(std::initializer_list<std::string> args) {
        storage.assign(args);
        for (auto& s : storage)
            ptrs.push_back(s.data());
    }
    int argc() const { return static_cast<int>(ptrs.size()); }
    char** argv() { return ptrs.data(); }
};
