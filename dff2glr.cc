#include <algorithm>
#include <cassert>
#include <numeric>

#include <GL/gl.h>
#include <rw.h>
#include <rwgta.h>

#include "base64.h"
#include "io.h"
#include "lodepng.h"

std::pair<std::string, std::string> printAtomic(rw::Atomic *atomic) {
    rw::Geometry *geom = atomic->geometry;
    char *name = gta::getNodeName(atomic->getFrame());

    std::string out = "";
    std::string named = "";

    assert(geom->morphTargets->vertices);
    size_t positionsLength = geom->numVertices * sizeof(rw::V3d);
    std::string uriPositions = "data:application/gltf-buffer;base64," +
        base64_encode((const unsigned char*)geom->morphTargets->vertices, positionsLength);
    named += ssprintf("\"%s.positions\": {\"uri\": \"%s\", \"byteLength\": %lu}\n",
            name, uriPositions.c_str(), positionsLength);

    if (geom->morphTargets->normals) {
        size_t normalsLength = geom->numVertices * sizeof(rw::V3d);
        std::string uriNormals = "data:application/gltf-buffer;base64," +
            base64_encode((const unsigned char*)geom->morphTargets->normals, normalsLength);
        named += ssprintf(", \"%s.normals\": {\"uri\": \"%s\", \"byteLength\": %lu}\n",
                name, uriNormals.c_str(), normalsLength);
    }

    if (geom->numTexCoordSets) {
        size_t texCoordsLength = geom->numVertices * sizeof(rw::TexCoords);
        std::string uriTexCoords = "data:application/gltf-buffer;base64," +
            base64_encode((const unsigned char*)geom->texCoords[0], texCoordsLength);
        named += ssprintf(", \"%s.texCoords\": {\"uri\": \"%s\", \"byteLength\": %lu}\n",
                name, uriTexCoords.c_str(), texCoordsLength);
    }

    if (geom->colors) {
        size_t colorsLength = geom->numVertices * sizeof(rw::RGBA);
        std::string uriColors = "data:application/gltf-buffer;base64," +
            base64_encode((const unsigned char*)geom->colors, colorsLength);
        named += ssprintf(", \"%s.colors\": {\"uri\": \"%s\", \"byteLength\": %lu}\n",
                name, uriColors.c_str(), colorsLength);
    }

    out += ssprintf("{\"name\": \"%s\", \"mesh\": {\"primitives\": [\n", name);
    rw::MeshHeader *h = geom->meshHeader;
    for (int i = 0; i < h->numMeshes; i++) {
        rw::Mesh *m = h->getMeshes() + i;
        if (i > 0) out += ssprintf(",");

        out += ssprintf("{\"mode\": %d", (h->flags & rw::MeshHeader::TRISTRIP) ? GL_TRIANGLE_STRIP : GL_TRIANGLES);
        out += ssprintf(", \"attributes\": ");
        out += ssprintf("{ \"POSITION\": ");
        out += ssprintf("{\"bufferView\": {\"buffer\": ");
        out += ssprintf("{\"name\": \"%s.positions\"}", name);
        out += ssprintf(", \"target\": %d, \"byteLength\": %lu}\n",
                GL_ARRAY_BUFFER, positionsLength);

        out += ssprintf(", \"type\": \"VEC3\", \"componentType\": %d, \"count\": %lu",
                GL_FLOAT, geom->numVertices);
        out += ssprintf(", \"min\": [-9,-9,-9]");
        out += ssprintf(", \"max\": [9,9,9]");
        out += ssprintf("}\n");

        if (geom->morphTargets->normals) {
            out += ssprintf(", \"NORMAL\": ");
            size_t normalsLength = geom->numVertices * sizeof(rw::V3d);

            out += ssprintf("{\"bufferView\": {\"buffer\": ");
            out += ssprintf("{\"name\": \"%s.normals\"}", name);
            out += ssprintf(", \"target\": %d, \"byteLength\": %lu}\n",
                    GL_ARRAY_BUFFER, normalsLength);

            out += ssprintf(", \"type\": \"VEC3\", \"componentType\": %d, \"count\": %lu",
                    GL_FLOAT, geom->numVertices);
            out += ssprintf("}\n");
        }

        if (geom->numTexCoordSets) {
            out += ssprintf(", \"TEXCOORD_0\": ");
            size_t texCoordsLength = geom->numVertices * sizeof(rw::TexCoords);

            out += ssprintf("{\"bufferView\": {\"buffer\": ");
            out += ssprintf("{\"name\": \"%s.texCoords\"}", name);
            out += ssprintf(", \"target\": %d, \"byteLength\": %lu}\n",
                    GL_ARRAY_BUFFER, texCoordsLength);

            out += ssprintf(", \"type\": \"VEC2\", \"componentType\": %d, \"count\": %lu",
                    GL_FLOAT, geom->numVertices);
            out += ssprintf("}\n");
        }

        if (geom->colors) {
            out += ssprintf(", \"COLOR_0\": ");
            size_t colorsLength = geom->numVertices * sizeof(rw::RGBA);

            out += ssprintf("{\"bufferView\": {\"buffer\": ");
            out += ssprintf("{\"name\": \"%s.colors\"}", name);
            out += ssprintf(", \"target\": %d, \"byteLength\": %lu}\n",
                    GL_ARRAY_BUFFER, colorsLength);

            out += ssprintf(", \"type\": \"VEC4\", \"componentType\": %d, \"count\": %lu",
                    GL_UNSIGNED_BYTE, geom->numVertices);
            out += ssprintf(", \"normalized\": true");
            out += ssprintf("}\n");
        }
        out += ssprintf("}");

        size_t indicesLength = m->numIndices * sizeof(uint16_t);
        std::string uriIndices = "data:application/gltf-buffer;base64," +
            base64_encode((const unsigned char*)m->indices, indicesLength);
        named += ssprintf(", \"%s.indices_%d\": {\"uri\": \"%s\", \"byteLength\": %lu}\n",
                name, i, uriIndices.c_str(), indicesLength);

        out += ssprintf(", \"indices\": {\"bufferView\": {\"buffer\": ");
        out += ssprintf("{\"name\": \"%s.indices_%d\"}", name, i);
        out += ssprintf(", \"target\": %d, \"byteLength\": %lu}\n",
                GL_ELEMENT_ARRAY_BUFFER, indicesLength);
        out += ssprintf(", \"type\": \"SCALAR\", \"componentType\": %d, \"count\": %lu}\n",
                GL_UNSIGNED_SHORT, m->numIndices);

        rw::Material *mat = m->material;
        rw::RGBA c = mat->color;
        out += ssprintf(", \"material\": {");
        out += ssprintf("\"pbrMetallicRoughness\": {");
        out += ssprintf("\"metallicFactor\": 0");
        if (mat->texture) {
            rw::Image *img = mat->texture->raster->toImage();
            img->unindex();

            if (img->depth < 24) {
                fprintf(stderr, "Warning: ignoring 16-bit texture %s for atomic %s\n", mat->texture->name, name);
            } else {
                bytes png;
                lodepng::encode(png, img->pixels, img->width, img->height, img->hasAlpha() ? LCT_RGBA : LCT_RGB);
                std::string uriPNG = "data:image/png;base64," + base64_encode(png.data(), png.size());

                out += ssprintf(", \"baseColorTexture\": {\"index\": {\"source\": ");
                out += ssprintf("{\"name\": \"%s\", \"uri\": \"%s\"}",
                        mat->texture->name, uriPNG.c_str());
                out += ssprintf(", \"sampler\": ");
                GLint filterConvMap[] = { 0, GL_NEAREST, GL_LINEAR, GL_NEAREST, GL_LINEAR, GL_NEAREST, GL_LINEAR };
                out += ssprintf("{\"minFilter\": %d, \"magFilter\": %d, ",
                        GL_LINEAR_MIPMAP_LINEAR, filterConvMap[mat->texture->getFilter()]);
                GLint addressConvMap[] = { 0, GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP, GL_CLAMP_TO_BORDER };
                out += ssprintf("\"wrapS\": %d, \"wrapT\": %d}\n",
                        addressConvMap[mat->texture->getAddressU()],
                        addressConvMap[mat->texture->getAddressV()]);
                out += ssprintf("}}");
            }

            img->destroy();
        }
        out += ssprintf(", \"baseColorFactor\": [%g,%g,%g,%g]",
                c.red / 255.0f, c.green / 255.0f, c.blue / 255.0f, c.alpha / 255.0f);
        out += ssprintf("}");
        if (c.alpha < 0xff) {
            out += ssprintf(", \"alphaMode\": \"BLEND\"");
        } else if (mat->texture) {
            out += ssprintf(", \"alphaMode\": \"%s\"", rw::Raster::formatHasAlpha(mat->texture->raster->format) ? "BLEND" : "OPAQUE");
        }
        out += ssprintf(", ");
        out += ssprintf("\"doubleSided\": true}");
        out += ssprintf("}\n");
    }
    out += ssprintf("]}}");

    return std::make_pair(out, named);
}

std::string printModel(rw::Clump *model) {
    std::string out = "";
    out += ssprintf("{\"asset\": {\"generator\": \"dff2gltf\", \"version\": \"2.0\"},\n");
    out += ssprintf("\"scene\": {\"nodes\": [{\"children\": [");
    std::string named = "";
    for (rw::LLLink *lnk = model->atomics.link.next; lnk != model->atomics.end(); lnk = lnk->next) {
        rw::Atomic *atomic = rw::Atomic::fromClump(lnk);
        std::string json, bufs;
        std::tie(json, bufs) = printAtomic(atomic);
        out += json;
        named += bufs;
        if (lnk->next != model->atomics.end()) {
            out += ssprintf(",");
            named += ",";
        }
    }
    out += ssprintf("]");
    if (model->getFrame()) {
        out += ssprintf(", \"name\": \"%s\"", gta::getNodeName(model->getFrame()));
    }
    out += ssprintf(", \"rotation\": [-0.5,0.5,0.5,0.5]}]}\n");
    out += ssprintf(", \"named\": {%s}", named.c_str());
    out += ssprintf("}\n");
    return out;
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
extern "C" {
    EMSCRIPTEN_KEEPALIVE const char *printModelC(rw::Clump *model) {
        return printModel(model).c_str();
    }
}
#else
int main(int argc, char **argv) {
    rw::Engine::init();
    gta::attachPlugins();
    rw::Engine::open();
    rw::Engine::start(nil);
    rw::Texture::setLoadTextures(false);

    std::string dffName(argv[1]);
    std::string txdName(argv[2]);
    assert(getenv("GTA3"));
    std::string gta3 = getenv("GTA3");
    gta3.erase(std::remove(gta3.begin(), gta3.end(), '\\'), gta3.end());
    auto dirPath = gta3 + "/models/gta3.dir";
    auto assets = loadDIR(readfile(dirPath));

    auto imgPath = gta3 + "/models/gta3.img";

    bytes txdata;
    if (txdName == "generic") {
        readfile(gta3 + "/models/generic.txd", txdata);
    } else {
        auto asset = findEntry(assets, txdName + ".txd");
        readimg(imgPath, asset, txdata);
    }

    rw::StreamMemory in;
    in.open(txdata.data(), txdata.size());

    rw::ChunkHeaderInfo header;
    readChunkHeaderInfo(&in, &header);
    assert(header.type == rw::ID_TEXDICTIONARY);

    rw::TexDictionary *txd = rw::TexDictionary::streamRead(&in);
    assert(txd);
    in.close();
    rw::TexDictionary::setCurrent(txd);

    auto asset = findEntry(assets, dffName + ".dff");
    auto data = readimg(imgPath, asset);
    in.open(data.data(), data.size());

    rw::currentUVAnimDictionary = NULL;
    readChunkHeaderInfo(&in, &header);
    if (header.type == rw::ID_UVANIMDICT) {
        rw::UVAnimDictionary *dict = rw::UVAnimDictionary::streamRead(&in);
        rw::currentUVAnimDictionary = dict;
        readChunkHeaderInfo(&in, &header);
    }
    if (header.type != rw::ID_CLUMP) {
        in.close();
        return 0;
    }
    rw::Clump *c = rw::Clump::streamRead(&in);
    in.close();
    assert(c);

    auto out = printModel(c);
    c->destroy();
    printf("%s", out.c_str());
}
#endif
