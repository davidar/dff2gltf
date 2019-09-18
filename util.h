#include <string>
#include <vector>

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
