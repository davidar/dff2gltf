#pragma once
#include <string>
#include <vector>

using bytes = std::vector<unsigned char>;

bytes to_bytes(const std::string &s);

inline void die(const std::string &s) {
    fprintf(stderr, "Error: %s\n", s.c_str());
    abort();
}
