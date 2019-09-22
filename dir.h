#pragma once
#include <string>
#include <vector>

#include "common.h"

struct DirEntry { // https://gtamods.com/wiki/IMG_archive
    uint32_t offset, size;
    char name[24];
};

std::vector<DirEntry> loadDIR(const bytes &data);

DirEntry findEntry(const std::vector<DirEntry> &assets, const std::string &fname);
