// https://github.com/rwengine/openrw/blob/master/rwcore/loaders/LoaderIMG.cpp

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "io.h"

int main(int argc, char **argv) {
    std::string basename(argv[1]);

    auto dirPath = basename + ".dir";
    auto assets = loadDIR(readfile(dirPath));

    auto imgPath = basename + ".img";

    for (auto &asset : assets) {
        auto raw_data = readimg(imgPath, asset);

        FILE* dumpFile = fopen(asset.name, "wb");
        if (!dumpFile) {
            fprintf(stderr, "Error: cannot open file %s\n", asset.name);
            return 1;
        }

        fwrite(raw_data.data(), 2048, asset.size, dumpFile);
        printf("Saved %s\n", asset.name);

        fclose(dumpFile);
    }
}
