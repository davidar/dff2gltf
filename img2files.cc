// https://github.com/rwengine/openrw/blob/master/rwcore/loaders/LoaderIMG.cpp

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "util.h"

class LoaderIMGFile {
public:
    uint32_t offset;
    uint32_t size;
    char name[24];
};


int main(int argc, char **argv) {
    std::string basename(argv[1]);

    auto dirPath = basename + ".dir";
    std::vector<LoaderIMGFile> assets;
    readfile(dirPath, assets);

    auto imgPath = basename + ".img";
    FILE* fp = fopen(imgPath.c_str(), "rb");
    if (!fp) return 1;

    for (auto &asset : assets) {
        auto raw_data = std::make_unique<char[]>(asset.size * 2048);

        fseek(fp, asset.offset * 2048, SEEK_SET);
        if (fread(raw_data.get(), 2048, asset.size, fp) != asset.size) {
            printf("Error reading asset %s\n", asset.name);
        }

        FILE* dumpFile = fopen(asset.name, "wb");
        if (!dumpFile) return 1;

        fwrite(raw_data.get(), 2048, asset.size, dumpFile);
        printf("Saved %s\n", asset.name);

        fclose(dumpFile);
    }

    fclose(fp);
}
