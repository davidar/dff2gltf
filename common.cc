#include "common.h"

bytes to_bytes(const std::string &s) {
    bytes data(s.begin(), s.end());
    return data;
}

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
using namespace emscripten;
EMSCRIPTEN_BINDINGS(common) {
    register_vector<unsigned char>("bytes");
    function("to_bytes", &to_bytes);
}
#endif
