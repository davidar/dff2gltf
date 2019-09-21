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

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
using namespace emscripten;
EMSCRIPTEN_BINDINGS(txd) {
    value_object<Texture>("Texture")
        .field("name",        &Texture::name)
        .field("alpha",       &Texture::alpha)
        .field("png",         &Texture::png)
        .field("transparent", &Texture::transparent)
        .field("minFilter",   &Texture::minFilter)
        .field("magFilter",   &Texture::magFilter)
        .field("wrapS",       &Texture::wrapS)
        .field("wrapT",       &Texture::wrapT)
        ;
    register_vector<unsigned char>("Buffer");
    register_vector<Texture>("TextureList");
    function("loadTXD", select_overload<std::vector<Texture>(const std::string &)>(&loadTXD));
    function("dataURI", &dataURI);
}
#endif
#endif
