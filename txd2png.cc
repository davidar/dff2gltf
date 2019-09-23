#include <cassert>

#include <rw.h>

#include "io.h"
#include "lodepng.h"

int main(int argc, char **argv) {
    rw::Engine::init();
    rw::Engine::open();
    rw::Engine::start(nil);
    rw::Texture::setLoadTextures(false);

    std::string fname(argv[1]);
    bytes data;
    readfile(fname, data);

    rw::StreamMemory in;
    in.open(data.data(), data.size());

    rw::ChunkHeaderInfo header;
    readChunkHeaderInfo(&in, &header);
    assert(header.type == rw::ID_TEXDICTIONARY);

    rw::TexDictionary *txd = rw::TexDictionary::streamRead(&in);
    assert(txd);
    in.close();
    rw::TexDictionary::setCurrent(txd);

    for (rw::LLLink *lnk = txd->textures.link.next; lnk != txd->textures.end(); lnk = lnk->next) {
        rw::Texture *tex = rw::Texture::fromDict(lnk);
        rw::Image *img = tex->raster->toImage();
        img->unindex();

        if (img->depth < 24) {
            fprintf(stderr, "Warning: ignoring %s\n", tex->name);
        } else {
            bytes png;
            lodepng::encode(png, img->pixels, img->width, img->height, img->hasAlpha() ? LCT_RGBA : LCT_RGB);
            lodepng::save_file(png, std::string(tex->name) + ".png");
        }

        img->destroy();
    }
    return 0;
}
