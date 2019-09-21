#include <cstring>
#include <memory>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/fetch.h>
#endif

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
void readimg(const std::string &fname, const DirEntry &asset, std::vector<T> &vec) {
#ifdef __EMSCRIPTEN__
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    std::string range = "bytes=" + std::to_string(2048 * asset.offset) + "-" + std::to_string(2048 * (asset.offset + asset.size));
    std::vector<const char *> headers = {"Range", range.c_str(), NULL};
    attr.requestHeaders = headers.data();
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS | EMSCRIPTEN_FETCH_REPLACE;
    emscripten_fetch_t *fetch = emscripten_fetch(&attr, fname.c_str());
    if (fetch->status == 206) {
        printf("Finished downloading %llu bytes from URL %s#%s.\n", fetch->numBytes, fetch->url, asset.name);
        vec.resize(fetch->numBytes / sizeof(T));
        memcpy(vec.data(), fetch->data, fetch->numBytes);
    } else {
        printf("Downloading %s (%s) failed, HTTP failure status code: %d.\n", fetch->url, range.c_str(), fetch->status);
    }
    emscripten_fetch_close(fetch);
#else
    FILE* fp = fopen(fname.c_str(), "rb");
    if (!fp) throw "cannot open file " + fname;
    vec.resize(asset.size * 2048);
    fseek(fp, asset.offset * 2048, SEEK_SET);
    if (fread(vec.data(), 2048, asset.size, fp) != asset.size) {
        printf("Error reading asset %s\n", asset.name);
    }
#endif
}

template <class T>
void readfile(const std::string &fname, std::vector<T> &vec) {
#ifdef __EMSCRIPTEN__
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS | EMSCRIPTEN_FETCH_REPLACE;
    printf("Fetching %s\n", fname.c_str());
    emscripten_fetch_t *fetch = emscripten_fetch(&attr, fname.c_str());
    if (fetch->status == 200) {
        printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
        vec.resize(fetch->numBytes / sizeof(T));
        memcpy(vec.data(), fetch->data, fetch->numBytes);
    } else {
        printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
    }
    emscripten_fetch_close(fetch);
#else
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
#endif
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
