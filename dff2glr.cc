#include <algorithm>
#include <numeric>

#include <glm/glm.hpp>

#include "base64.h"
#include "dff.h"
#include "io.h"
#include "txd.h"

std::string printAttribute(int i, const AtomicPtr &atomic) {
    auto &geom = atomic->getGeometry();
    auto &name = atomic->getFrame()->getName();
    glm::vec3 minPos = std::accumulate(
        std::next(geom->verts.begin()), geom->verts.end(), geom->verts[0].position,
        [](glm::vec3 a, const GeometryVertex &v) { return glm::min(a, v.position); });
    glm::vec3 maxPos = std::accumulate(
        std::next(geom->verts.begin()), geom->verts.end(), geom->verts[0].position,
        [](glm::vec3 a, const GeometryVertex &v) { return glm::max(a, v.position); });

    std::string out = "";
    out += ssprintf("{\"bufferView\": {\"buffer\": ");
    size_t attrLength = sizeof(GeometryVertex) * geom->verts.size();
    out += ssprintf("{\"name\": \"%s.attributes\"}", name.c_str());
    out += ssprintf(", \"target\": %d, \"byteLength\": %lu, \"byteStride\": %lu}\n",
            GL_ARRAY_BUFFER, attrLength, sizeof(GeometryVertex));

    auto attr = GeometryVertex::vertex_attributes()[i];
    out += ssprintf(", \"type\": \"VEC%d\", \"byteOffset\": %lu, \"componentType\": %d, \"count\": %lu",
            attr.size, attr.offset, attr.type, geom->verts.size());
    if (attr.sem == ATRS_Position) {
        out += ssprintf(", \"min\": [%.9g,%.9g,%.9g]", minPos.x, minPos.y, minPos.z);
        out += ssprintf(", \"max\": [%.9g,%.9g,%.9g]", maxPos.x, maxPos.y, maxPos.z);
    } else if (attr.sem == ATRS_Colour) {
        out += ssprintf(", \"normalized\": true");
    }
    out += ssprintf("}\n");
    return out;
}

std::pair<std::string, std::string> printAtomic(const AtomicPtr &atomic, const std::vector<Texture> &textures) {
    auto &geom = atomic->getGeometry();
    auto &name = atomic->getFrame()->getName();

    size_t attrLength = sizeof(GeometryVertex) * geom->verts.size();
    std::string uriAttr = "data:application/gltf-buffer;base64," +
        base64_encode((const unsigned char*)geom->verts.data(), attrLength);
    std::string named = "\"" + name + ".attributes\": " +
        "{\"uri\": \"" + uriAttr + "\", \"byteLength\": " + std::to_string(attrLength) + "}";

    std::vector<uint32_t> indices;
    for (auto &sg : geom->subgeom) {
        indices.insert(indices.end(), sg.indices.begin(), sg.indices.end());
    }
    auto indicesLength = indices.size() * sizeof(uint32_t);
    auto uriIndices = "data:application/gltf-buffer;base64," +
        base64_encode((const unsigned char*)indices.data(), indicesLength);
    named += ", \"" + name + ".indices\": " +
        "{\"uri\": \"" + uriIndices + "\", \"byteLength\": " + std::to_string(indicesLength) + "}";

    std::string out = "";
    out += ssprintf("{\"name\": \"%s\", \"mesh\": {\"primitives\": [\n", name.c_str());
    for (auto const &sg : geom->subgeom) {
        out += ssprintf("{\"mode\": %d", geom->facetype == Geometry::Triangles ? GL_TRIANGLES : GL_TRIANGLE_STRIP);
        out += ssprintf(", \"attributes\": ");
        out += ssprintf("{ \"POSITION\": ");
        out += printAttribute(0, atomic);
        out += ssprintf(", \"NORMAL\": ");
        out += printAttribute(1, atomic);
        out += ssprintf(", \"TEXCOORD_0\": ");
        out += printAttribute(2, atomic);
        out += ssprintf(", \"COLOR_0\": ");
        out += printAttribute(3, atomic);
        out += ssprintf("}");
        out += ssprintf(", \"indices\": {\"bufferView\": {\"buffer\": ");
        out += ssprintf("{\"name\": \"%s.indices\"}", name.c_str());
        out += ssprintf(", \"target\": %d, \"byteOffset\": %lu, \"byteLength\": %lu}\n",
                GL_ELEMENT_ARRAY_BUFFER, sg.start * sizeof(uint32_t), sizeof(uint32_t) * sg.numIndices);
        out += ssprintf(", \"type\": \"SCALAR\", \"componentType\": %d, \"count\": %lu}\n",
                GL_UNSIGNED_INT, sg.numIndices);

        int matIndex = sg.material;
        auto const &mat = geom->materials[matIndex];
        glm::vec4 c(-1);
        if ((geom->flags & RW::BSGeometry::ModuleMaterialColor) ==
                RW::BSGeometry::ModuleMaterialColor) {
            c = glm::vec4(mat.colour) / 255.0f;
        }
        out += ssprintf(", \"material\": {");
        if (!mat.textures.empty() || c != glm::vec4(-1)) {
            const Texture *texture = NULL;
            for (int i = 0; i < textures.size(); i++) {
                if (!mat.textures.empty() && textures[i].name == mat.textures[0].name) {
                    texture = &textures[i];
                    break;
                }
            }
            out += ssprintf("\"pbrMetallicRoughness\": {");
            out += ssprintf("\"metallicFactor\": 0");
            if (texture) {
                out += ssprintf(", \"baseColorTexture\": {\"index\": {\"source\": ");
                out += ssprintf("{\"name\": \"%s\", \"uri\": \"%s\"}",
                        texture->name.c_str(), dataURI(*texture).c_str());
                out += ssprintf(", \"sampler\": ");
                out += ssprintf("{\"minFilter\": %d, \"magFilter\": %d, ",
                        texture->minFilter, texture->magFilter);
                out += ssprintf("\"wrapS\": %d, \"wrapT\": %d}\n",
                        texture->wrapS, texture->wrapT);
                out += ssprintf("}}");
            } else if (!mat.textures.empty()) {
                fprintf(stderr, "Warning: missing texture %s\n", mat.textures[0].name.c_str());
            }
            if (c != glm::vec4(-1)) {
                out += ssprintf(", \"baseColorFactor\": [%g,%g,%g,%g]", c.r,c.g,c.b,c.a);
            }
            out += ssprintf("}");
            if (c != glm::vec4(-1) && c.a < 1) {
                out += ssprintf(", \"alphaMode\": \"BLEND\"");
            } else if (texture) {
                out += ssprintf(", \"alphaMode\": \"%s\"", texture->transparent ? "BLEND" : "OPAQUE");
            }
            out += ssprintf(", ");
        }
        out += ssprintf("\"doubleSided\": true}");
        out += ssprintf("}\n");
        if (&sg != &geom->subgeom.back()) out += ssprintf(",");
    }
    out += ssprintf("]}}");

    return std::make_pair(out, named);
}

std::string printModel(const ClumpPtr model, const std::vector<Texture> &textures) {
    std::string out = "";
    out += ssprintf("{\"asset\": {\"generator\": \"dff2gltf\", \"version\": \"2.0\"},\n");
    out += ssprintf("\"scene\": {\"nodes\": [{\"children\": [");
    std::string named = "";
    for (auto const &atomic : model->getAtomics()) {
        std::string json, bufs;
        std::tie(json, bufs) = printAtomic(atomic, textures);
        out += json;
        named += bufs;
        if (&atomic != &model->getAtomics().back()) {
            out += ssprintf(",");
            named += ",";
        }
    }
    out += ssprintf("]");
    if (model->getFrame()) {
        out += ssprintf(", \"name\": \"%s\"", model->getFrame()->getName().c_str());
    }
    out += ssprintf(", \"rotation\": [-0.5,0.5,0.5,0.5]}]}");
    out += ssprintf(", \"named\": {%s}", named.c_str());
    out += ssprintf("}\n");
    return out;
}

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
using namespace emscripten;
EMSCRIPTEN_BINDINGS(dff2glr) {
    function("printModel", &printModel);
}
#else
int main(int argc, char **argv) {
    std::string dff(argv[1]);
    std::string txd(argv[2]);
        std::string gta3 = getenv("GTA3");
        gta3.erase(std::remove(gta3.begin(), gta3.end(), '\\'), gta3.end());
        auto dirPath = gta3 + "/models/gta3.dir";
        auto assets = loadDIR(readfile(dirPath));

        auto imgPath = gta3 + "/models/gta3.img";

        auto asset = findEntry(assets, dff + ".dff");
        auto model = loadDFF(readimg(imgPath, asset));

        bytes txdata;
        if (txd == "generic") {
            readfile(gta3 + "/models/generic.txd", txdata);
        } else {
            auto asset = findEntry(assets, txd + ".txd");
            readimg(imgPath, asset, txdata);
        }
        auto textures = loadTXD(txdata);

        auto out = printModel(model, textures);
        printf("%s", out.c_str());
}
#endif
