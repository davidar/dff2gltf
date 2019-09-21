// https://github.com/rwengine/openrw/blob/master/rwcore/loaders/LoaderTXD.cpp
#include "txd.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>

#include "base64.h"
#include "lodepng.h"
#include "RWBinaryStream.hpp"

const size_t paletteSize = 1024;

static
void processPalette(uint32_t* fullColor, RW::BinaryStreamSection& rootSection) {
    const uint8_t* dataBase = reinterpret_cast<const uint8_t*>(
        rootSection.raw() + sizeof(RW::BSSectionHeader) +
        sizeof(RW::BSTextureNative) - 4);

    const uint8_t* coldata = (dataBase + paletteSize + sizeof(uint32_t));
    uint32_t raster_size = *reinterpret_cast<const uint32_t*>(dataBase + paletteSize);
    const uint32_t* palette = reinterpret_cast<const uint32_t*>(dataBase);

    for (size_t j = 0; j < raster_size; ++j) {
        *(fullColor++) = palette[coldata[j]];
    }
}

std::vector<Texture> loadTXD(const std::vector<char> &data) {
    std::vector<Texture> textures;
    RW::BinaryStreamSection root(data.data());
    /*auto texDict =*/root.readStructure<RW::BSTextureDictionary>();

    size_t rootI = 0;
    while (root.hasMoreData(rootI)) {
        Texture texture;
        auto rootSection = root.getNextChildSection(rootI);

        if (rootSection.header.id != RW::SID_TextureNative) continue;

        RW::BSTextureNative texNative =
            rootSection.readStructure<RW::BSTextureNative>();
        texture.name = std::string(texNative.diffuseName);
        texture.alpha = std::string(texNative.alphaName);
        std::transform(texture.name.begin(), texture.name.end(), texture.name.begin(), ::tolower);
        std::transform(texture.alpha.begin(), texture.alpha.end(), texture.alpha.begin(), ::tolower);

        if (texNative.platform != 8) {
            printf("Error: unsupported texture platform %d\n", texNative.platform);
            break;
        }

        bool isPal8 =
            (texNative.rasterformat & RW::BSTextureNative::FORMAT_EXT_PAL8) ==
            RW::BSTextureNative::FORMAT_EXT_PAL8;
        bool isFulc = texNative.rasterformat == RW::BSTextureNative::FORMAT_1555 ||
                      texNative.rasterformat == RW::BSTextureNative::FORMAT_8888 ||
                      texNative.rasterformat == RW::BSTextureNative::FORMAT_888;
        // Export this value
        texture.transparent =
            !((texNative.rasterformat & RW::BSTextureNative::FORMAT_888) ==
              RW::BSTextureNative::FORMAT_888);

        if (!(isPal8 || isFulc)) {
            printf("Error: unsupported raster format %d\n", texNative.rasterformat);
            break;
        }

        if (isPal8) {
            std::vector<uint32_t> fullColor(texNative.width * texNative.height);

            processPalette(fullColor.data(), rootSection);

            lodepng::encode(texture.png, (unsigned char*)fullColor.data(), texNative.width, texNative.height);
        } else if (isFulc) {
            auto coldata = rootSection.raw() + sizeof(RW::BSTextureNative);
            coldata += sizeof(uint32_t);

            bool bgra = false;
            switch (texNative.rasterformat) {
                case RW::BSTextureNative::FORMAT_1555:
                    //type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
                    printf("Warning: ignoring %s\n", texture.name.c_str());
                    continue;
                    break;
                case RW::BSTextureNative::FORMAT_8888:
                    bgra = true;
                    // type = GL_UNSIGNED_INT_8_8_8_8_REV;
                    coldata += 8;
                    break;
                case RW::BSTextureNative::FORMAT_888:
                    bgra = true;
                    break;
                default:
                    break;
            }

            int len = texNative.width * texNative.height;
            std::vector<unsigned char> rgba(coldata, coldata + 4*len);

            if (bgra) {
                for (int i = 0; i < len; i++) {
                    std::swap(rgba[4*i], rgba[4*i+2]);
                }
            }

            lodepng::encode(texture.png, rgba, texNative.width, texNative.height);
        } else {
            assert(0);
        }

        texture.minFilter = GL_LINEAR_MIPMAP_LINEAR;
        switch (texNative.filterflags & 0xFF) {
            default:
            case RW::BSTextureNative::FILTER_LINEAR:
                texture.magFilter = GL_LINEAR;
                break;
            case RW::BSTextureNative::FILTER_NEAREST:
                texture.magFilter = GL_NEAREST;
                break;
        }

        switch (texNative.wrapU) {
            default:
            case RW::BSTextureNative::WRAP_WRAP:
                texture.wrapS = GL_REPEAT;
                break;
            case RW::BSTextureNative::WRAP_CLAMP:
                texture.wrapS = GL_CLAMP_TO_EDGE;
                break;
            case RW::BSTextureNative::WRAP_MIRROR:
                texture.wrapS = GL_MIRRORED_REPEAT;
                break;
        }

        switch (texNative.wrapV) {
            default:
            case RW::BSTextureNative::WRAP_WRAP:
                texture.wrapT = GL_REPEAT;
                break;
            case RW::BSTextureNative::WRAP_CLAMP:
                texture.wrapT = GL_CLAMP_TO_EDGE;
                break;
            case RW::BSTextureNative::WRAP_MIRROR:
                texture.wrapT = GL_MIRRORED_REPEAT;
                break;
        }

        textures.push_back(texture);
    }

    return textures;
}

std::vector<Texture> loadTXD(const std::string &s) {
    std::vector<char> data(s.begin(), s.end());
    return loadTXD(data);
}

std::string dataURI(const Texture &texture) {
    return "data:image/png;base64," + base64_encode(texture.png.data(), texture.png.size());
}

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
