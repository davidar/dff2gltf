#include <string>
#include <vector>

template <class T>
void readfile(const std::string &fname, std::vector<T> &vec) {
    FILE* fp = fopen(fname.c_str(), "rb");

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
