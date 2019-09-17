// https://github.com/rwengine/openrw/blob/master/rwcore/loaders/LoaderTXD.cpp

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <GL/gl.h>
#include <Magick++.h>

#include "RWBinaryStream.hpp"
#include "util.h"

const size_t paletteSize = 1024;

static
void processPalette(uint32_t* fullColor, RW::BinaryStreamSection& rootSection) {
    uint8_t* dataBase = reinterpret_cast<uint8_t*>(
        rootSection.raw() + sizeof(RW::BSSectionHeader) +
        sizeof(RW::BSTextureNative) - 4);

    uint8_t* coldata = (dataBase + paletteSize + sizeof(uint32_t));
    uint32_t raster_size = *reinterpret_cast<uint32_t*>(dataBase + paletteSize);
    uint32_t* palette = reinterpret_cast<uint32_t*>(dataBase);

    for (size_t j = 0; j < raster_size; ++j) {
        *(fullColor++) = palette[coldata[j]];
    }
}

int main(int argc, char **argv) {
    Magick::InitializeMagick(argv[0]);
    std::string fname(argv[1]);

    std::vector<char> data;
    readfile(fname, data);

    RW::BinaryStreamSection root(data.data());
    /*auto texDict =*/root.readStructure<RW::BSTextureDictionary>();

    size_t rootI = 0;
    while (root.hasMoreData(rootI)) {
        auto rootSection = root.getNextChildSection(rootI);

        if (rootSection.header.id != RW::SID_TextureNative) continue;

        RW::BSTextureNative texNative =
            rootSection.readStructure<RW::BSTextureNative>();
        std::string name = std::string(texNative.diffuseName);
        std::string alpha = std::string(texNative.alphaName);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::transform(alpha.begin(), alpha.end(), alpha.begin(), ::tolower);

        if (texNative.platform != 8) {
            printf("Unsupported texture platform %d\n", texNative.platform);
            return 1;
        }

        bool isPal8 =
            (texNative.rasterformat & RW::BSTextureNative::FORMAT_EXT_PAL8) ==
            RW::BSTextureNative::FORMAT_EXT_PAL8;
        bool isFulc = texNative.rasterformat == RW::BSTextureNative::FORMAT_1555 ||
                      texNative.rasterformat == RW::BSTextureNative::FORMAT_8888 ||
                      texNative.rasterformat == RW::BSTextureNative::FORMAT_888;
        // Export this value
        bool transparent =
            !((texNative.rasterformat & RW::BSTextureNative::FORMAT_888) ==
              RW::BSTextureNative::FORMAT_888);

        if (!(isPal8 || isFulc)) {
            printf("Unsupported raster format %d\n", texNative.rasterformat);
            return 1;
        }

        Magick::Image texture;

        if (isPal8) {
            std::vector<uint32_t> fullColor(texNative.width * texNative.height);

            processPalette(fullColor.data(), rootSection);

            texture.read(texNative.width, texNative.height,
                    "RGBA", Magick::CharPixel, fullColor.data());
        } else if (isFulc) {
            auto coldata = rootSection.raw() + sizeof(RW::BSTextureNative);
            coldata += sizeof(uint32_t);

            std::string format = "RGBA";
            switch (texNative.rasterformat) {
                case RW::BSTextureNative::FORMAT_1555:
                    format = "RGBA";
                    printf("Error: type = GL_UNSIGNED_SHORT_1_5_5_5_REV");
                    return 1;
                    break;
                case RW::BSTextureNative::FORMAT_8888:
                    format = "BGRA";
                    // type = GL_UNSIGNED_INT_8_8_8_8_REV;
                    coldata += 8;
                    break;
                case RW::BSTextureNative::FORMAT_888:
                    format = "BGRA";
                    break;
                default:
                    break;
            }

            texture.read(texNative.width, texNative.height,
                    format, Magick::CharPixel, coldata);
        } else {
            return 1;
        }

        texture.write(name + ".png"); 

        FILE* meta = fopen((name + ".txt").c_str(), "w");
        fprintf(meta, "%s", transparent ? "BLEND" : "OPAQUE");
        fclose(meta);

        meta = fopen((name + ".json").c_str(), "w");

        GLenum texFilter = GL_LINEAR;
        switch (texNative.filterflags & 0xFF) {
            default:
            case RW::BSTextureNative::FILTER_LINEAR:
                texFilter = GL_LINEAR;
                break;
            case RW::BSTextureNative::FILTER_NEAREST:
                texFilter = GL_NEAREST;
                break;
        }

        fprintf(meta, "{\"minFilter\": %d, ", GL_LINEAR_MIPMAP_LINEAR);
        fprintf(meta, "\"magFilter\": %d, ", texFilter);

        GLenum texwrap = GL_REPEAT;
        switch (texNative.wrapU) {
            default:
            case RW::BSTextureNative::WRAP_WRAP:
                texwrap = GL_REPEAT;
                break;
            case RW::BSTextureNative::WRAP_CLAMP:
                texwrap = GL_CLAMP_TO_EDGE;
                break;
            case RW::BSTextureNative::WRAP_MIRROR:
                texwrap = GL_MIRRORED_REPEAT;
                break;
        }
        fprintf(meta, "\"wrapS\": %d, ", texwrap);

        switch (texNative.wrapV) {
            default:
            case RW::BSTextureNative::WRAP_WRAP:
                texwrap = GL_REPEAT;
                break;
            case RW::BSTextureNative::WRAP_CLAMP:
                texwrap = GL_CLAMP_TO_EDGE;
                break;
            case RW::BSTextureNative::WRAP_MIRROR:
                texwrap = GL_MIRRORED_REPEAT;
                break;
        }
        fprintf(meta, "\"wrapT\": %d}\n", texwrap);
        fclose(meta);
    }

    return 0;
}
