// https://github.com/rwengine/openrw/blob/master/rwcore/loaders/LoaderIMG.cpp

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "io.h"

int main(int argc, char **argv) {
    std::string basename(argv[1]);

    auto dirPath = basename + ".dir";
    std::vector<DirEntry> assets;
    readfile(dirPath, assets);

    auto imgPath = basename + ".img";

    for (auto &asset : assets) {
        std::vector<char> raw_data;
        readimg(imgPath, asset, raw_data);

        FILE* dumpFile = fopen(asset.name, "wb");
        if (!dumpFile) return 1;

        fwrite(raw_data.data(), 2048, asset.size, dumpFile);
        printf("Saved %s\n", asset.name);

        fclose(dumpFile);
    }
}
