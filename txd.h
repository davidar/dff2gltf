#pragma once
#include <string>
#include <vector>

#include <GL/gl.h>

#include "common.h"

struct Texture {
    std::string name, alpha;
    bytes png;
    bool transparent;
    GLenum minFilter, magFilter, wrapS, wrapT;
};

std::vector<Texture> loadTXD(const bytes &data);
std::string dataURI(const Texture &texture);
