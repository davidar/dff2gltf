#include <memory>
#include <string>
#include <vector>

struct DirEntry { // https://gtamods.com/wiki/IMG_archive
    uint32_t offset, size;
    char name[24];
};

DirEntry findentry(const std::vector<DirEntry> &assets, const std::string &fname) {
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
    throw "cannot find " + fname;
}

template <class T>
void readimg(FILE* fp, const DirEntry &asset, std::vector<T> &vec) {
    vec.resize(asset.size * 2048);
    fseek(fp, asset.offset * 2048, SEEK_SET);
    if (fread(vec.data(), 2048, asset.size, fp) != asset.size) {
        printf("Error reading asset %s\n", asset.name);
    }
}

template <class T>
void readfile(const std::string &fname, std::vector<T> &vec) {
    FILE* fp = fopen(fname.c_str(), "rb");
    if (!fp) throw "cannot open file " + fname;

    fseek(fp, 0, SEEK_END);
    unsigned long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::size_t expectedCount = fileSize / sizeof(T);
    vec.resize(expectedCount);
    std::size_t actualCount = fread(vec.data(), sizeof(T), expectedCount, fp);

    if (expectedCount != actualCount) {
        vec.resize(actualCount);
        fprintf(stderr, "Error reading file\n");
    }

    fclose(fp);
}

void cat(const std::string &fname) {
    FILE* fp = fopen(fname.c_str(), "r");
    if (!fp) throw "cannot open file " + fname;
    char buf[1024];
    while (size_t buflen = fread(buf, 1, sizeof(buf), fp)) {
        fwrite(buf, 1, buflen, stdout);
    }
    fclose(fp);
}

bool exists(const std::string &fname) {
    FILE* fp;
    if ((fp = fopen(fname.c_str(), "r"))) {
        fclose(fp);
        return true;
    }
    return false;
}
