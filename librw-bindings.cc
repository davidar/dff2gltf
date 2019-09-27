#include <emscripten.h>
#include <rw.h>
#include <rwgta.h>

#define A EMSCRIPTEN_KEEPALIVE

extern "C" {
A void rw_currentUVAnimDictionary_set(rw::UVAnimDictionary *v)
    { rw::currentUVAnimDictionary = v; }
A bool rw_readChunkHeaderInfo(rw::Stream *s, rw::ChunkHeaderInfo *h)
    { return rw::readChunkHeaderInfo(s, h); }

A rw::ChunkHeaderInfo *rw_ChunkHeaderInfo_new() { return new rw::ChunkHeaderInfo(); }
A uint32_t rw_ChunkHeaderInfo_type(rw::ChunkHeaderInfo *self) { return self->type; }
A void rw_ChunkHeaderInfo_delete(rw::ChunkHeaderInfo *self) { delete self; }

A rw::Clump *rw_Clump_streamRead(rw::Stream *s) { return rw::Clump::streamRead(s); }
A void rw_Clump_destroy(rw::Clump *self) { self->destroy(); }

A bool rw_Engine_init() { return rw::Engine::init(); }
A bool rw_Engine_open() { return rw::Engine::open(); }
A bool rw_Engine_start(rw::EngineStartParams *p) { return rw::Engine::start(p); }

A void rw_Image_unindex(rw::Image *self) { self->unindex(); }
A int32_t rw_Image_bpp(rw::Image *self) { return self->bpp; }
A int32_t rw_Image_depth(rw::Image *self) { return self->depth; }
A uint8_t *rw_Image_pixels(rw::Image *self) { return self->pixels; }
A int32_t rw_Image_width(rw::Image *self) { return self->width; }
A int32_t rw_Image_height(rw::Image *self) { return self->height; }
A bool rw_Image_hasAlpha(rw::Image *self) { return self->hasAlpha(); }
A void rw_Image_destroy(rw::Image *self) { self->destroy(); }

A rw::LLLink *rw_LinkList_end(rw::LinkList *self) { return self->end(); }

A rw::LLLink *rw_LLLink_next(rw::LLLink *self) { return self->next; }

A rw::Image *rw_Raster_toImage(rw::Raster *self) { return self->toImage(); }

A rw::StreamMemory *rw_StreamMemory_new() { return new rw::StreamMemory(); }
A rw::StreamMemory *rw_StreamMemory_open(rw::StreamMemory *self, uint8_t *data, uint32_t length, uint32_t capacity)
    { return self->open(data, length, capacity); }
A void rw_StreamMemory_close(rw::StreamMemory *self) { self->close(); }
A void rw_StreamMemory_delete(rw::StreamMemory *self) { delete self; }

A rw::TexDictionary *rw_TexDictionary_streamRead(rw::Stream *s)
    { return rw::TexDictionary::streamRead(s); }
A void rw_TexDictionary_setCurrent(rw::TexDictionary *txd)
    { rw::TexDictionary::setCurrent(txd); }
A rw::LinkList *rw_TexDictionary_textures(rw::TexDictionary *self)
    { return &self->textures; }

A void rw_Texture_setCreateDummies(bool b) { rw::Texture::setCreateDummies(b); }
A void rw_Texture_setLoadTextures(bool b) { rw::Texture::setLoadTextures(b); }
A rw::Texture *rw_Texture_fromDict(rw::LLLink *lnk) { return rw::Texture::fromDict(lnk); }
A rw::Raster *rw_Texture_raster(rw::Texture *self) { return self->raster; }
A char *rw_Texture_name(rw::Texture *self) { return self->name; }

A rw::UVAnimDictionary *rw_UVAnimDictionary_streamRead(rw::Stream *s)
    { return rw::UVAnimDictionary::streamRead(s); }

A void gta_attachPlugins() { gta::attachPlugins(); }
}
