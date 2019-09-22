#include "dir.h"

#include <cstring>

std::vector<DirEntry> loadDIR(const bytes &data) {
    std::vector<DirEntry> assets;
    assets.resize(data.size() / sizeof(DirEntry));
    memcpy(assets.data(), data.data(), data.size());
    return assets;
}

DirEntry findEntry(const std::vector<DirEntry> &assets, const std::string &fname) {
    for (auto const &asset : assets) {
        bool equal = true;
        for (int i = 0; i < fname.size() && i < 24; i++) {
            if (std::tolower(asset.name[i]) != std::tolower(fname[i])) {
                equal = false;
                break;
            }
        }
        if (equal) return asset;
    }
    die("cannot find " + fname);
    abort();
}

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
using namespace emscripten;
EMSCRIPTEN_BINDINGS(dir) {
    value_object<DirEntry>("DirEntry")
        .field("offset", &DirEntry::offset)
        .field("size",   &DirEntry::size)
        ;
    register_vector<DirEntry>("DirEntryList");
    function("loadDIR", &loadDIR);
    function("findEntry", &findEntry);
}
#endif
