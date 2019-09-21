#ifndef _TXD_H_
#define _TXD_H_
#include <string>
#include <vector>

#include <GL/gl.h>

struct Texture {
    std::string name, alpha;
    std::vector<unsigned char> png;
    bool transparent;
    GLenum minFilter, magFilter, wrapS, wrapT;
};

std::vector<Texture> loadTXD(const std::vector<char> &data);
std::vector<Texture> loadTXD(const std::string &s);
std::string dataURI(const Texture &texture);
#endif
