#include <algorithm>
#include <numeric>

#include <glm/glm.hpp>

#include "base64.h"
#include "dff.h"
#include "io.h"
#include "txd.h"

void printAttribute(int i, const AtomicPtr &atomic, std::string prefix) {
    auto &geom = atomic->getGeometry();
    auto &name = atomic->getFrame()->getName();
    glm::vec3 minPos = std::accumulate(
        std::next(geom->verts.begin()), geom->verts.end(), geom->verts[0].position,
        [](glm::vec3 a, const GeometryVertex &v) { return glm::min(a, v.position); });
    glm::vec3 maxPos = std::accumulate(
        std::next(geom->verts.begin()), geom->verts.end(), geom->verts[0].position,
        [](glm::vec3 a, const GeometryVertex &v) { return glm::max(a, v.position); });

    printf("{\"bufferView\": {\"buffer\": ");
    size_t attrLength = sizeof(GeometryVertex) * geom->verts.size();
    printf("{\"uri\": \"%s.attributes.bin\", \"byteLength\": %lu}",
            (prefix + name).c_str(), attrLength);
    printf(", \"target\": %d, \"byteLength\": %lu, \"byteStride\": %lu}\n",
            GL_ARRAY_BUFFER, attrLength, sizeof(GeometryVertex));

    auto attr = GeometryVertex::vertex_attributes()[i];
    printf(", \"type\": \"VEC%d\", \"byteOffset\": %lu, \"componentType\": %d, \"count\": %lu",
            attr.size, attr.offset, attr.type, geom->verts.size());
    if (attr.sem == ATRS_Position) {
        printf(", \"min\": [%.9g,%.9g,%.9g]", minPos.x, minPos.y, minPos.z);
        printf(", \"max\": [%.9g,%.9g,%.9g]", maxPos.x, maxPos.y, maxPos.z);
    } else if (attr.sem == ATRS_Colour) {
        printf(", \"normalized\": true");
    }
    printf("}\n");
}

void printAtomic(const AtomicPtr &atomic, std::string prefix, const std::vector<Texture> &textures) {
    auto &geom = atomic->getGeometry();
    auto &name = atomic->getFrame()->getName();
    if (!prefix.empty()) prefix = prefix + ".";
    prefix = "buf/" + prefix;

    FILE* bin = fopen((prefix + name + ".attributes.bin").c_str(), "wb");
    fwrite(geom->verts.data(), sizeof(GeometryVertex), geom->verts.size(), bin);
    fclose(bin);

    bin = fopen((prefix + name + ".indices.bin").c_str(), "wb");
    for (auto &sg : geom->subgeom) {
        fwrite(sg.indices.data(), sizeof(uint32_t), sg.numIndices, bin);
    }
    fclose(bin);

    printf("{\"name\": \"%s\", \"mesh\": {\"primitives\": [\n", name.c_str());
    for (auto const &sg : geom->subgeom) {
        printf("{\"mode\": %d", geom->facetype == Geometry::Triangles ? GL_TRIANGLES : GL_TRIANGLE_STRIP);
        printf(", \"attributes\": ");
        printf("{ \"POSITION\": ");
        printAttribute(0, atomic, prefix);
        printf(", \"NORMAL\": ");
        printAttribute(1, atomic, prefix);
        printf(", \"TEXCOORD_0\": ");
        printAttribute(2, atomic, prefix);
        printf(", \"COLOR_0\": ");
        printAttribute(3, atomic, prefix);
        printf("}");
        printf(", \"indices\": {\"bufferView\": {\"buffer\": ");
        size_t icount = std::accumulate(
            geom->subgeom.begin(), geom->subgeom.end(), size_t{0u},
            [](size_t a, const SubGeometry &b) { return a + b.numIndices; });
        printf("{\"uri\": \"%s.indices.bin\", \"byteLength\": %lu}",
                (prefix + name).c_str(), sizeof(uint32_t) * icount);
        printf(", \"target\": %d, \"byteOffset\": %lu, \"byteLength\": %lu}\n",
                GL_ELEMENT_ARRAY_BUFFER, sg.start * sizeof(uint32_t), sizeof(uint32_t) * sg.numIndices);
        printf(", \"type\": \"SCALAR\", \"componentType\": %d, \"count\": %lu}\n",
                GL_UNSIGNED_INT, sg.numIndices);
        int matIndex = sg.material;
        auto const &mat = geom->materials[matIndex];
        glm::vec4 c(-1);
        if ((geom->flags & RW::BSGeometry::ModuleMaterialColor) ==
                RW::BSGeometry::ModuleMaterialColor) {
            c = glm::vec4(mat.colour) / 255.0f;
        }
        printf(", \"material\": {");
        if (!mat.textures.empty() || c != glm::vec4(-1)) {
            const Texture *texture = NULL;
            for (int i = 0; i < textures.size(); i++) {
                if (!mat.textures.empty() && textures[i].name == mat.textures[0].name) {
                    texture = &textures[i];
                    break;
                }
            }
            printf("\"pbrMetallicRoughness\": {");
            printf("\"metallicFactor\": 0");
            if (texture) {
                printf(", \"baseColorTexture\": {\"index\": {\"source\": ");
                auto uri = "data:image/png;base64," +
                        base64_encode(texture->png.data(), texture->png.size());
                printf("{\"name\": \"%s\", \"uri\": \"%s\"}",
                        texture->name.c_str(), uri.c_str());
                printf(", \"sampler\": ");
                printf("{\"minFilter\": %d, \"magFilter\": %d, ",
                        texture->minFilter, texture->magFilter);
                printf("\"wrapS\": %d, \"wrapT\": %d}\n",
                        texture->wrapS, texture->wrapT);
                printf("}}");
            } else if (!mat.textures.empty()) {
                fprintf(stderr, "Warning: missing texture %s\n", mat.textures[0].name.c_str());
            }
            if (c != glm::vec4(-1)) {
                printf(", \"baseColorFactor\": [%g,%g,%g,%g]", c.r,c.g,c.b,c.a);
            }
            printf("}");
            if (c != glm::vec4(-1) && c.a < 1) {
                printf(", \"alphaMode\": \"BLEND\"");
            } else if (texture) {
                printf(", \"alphaMode\": \"%s\"", texture->transparent ? "BLEND" : "OPAQUE");
            }
            printf(", ");
        }
        printf("\"doubleSided\": true}");
        printf("}\n");
        if (&sg != &geom->subgeom.back()) printf(",");
    }
    printf("]}}");
}

void printModel(const ClumpPtr model, const std::vector<Texture> &textures) {
    auto modelName = model->getFrame() ? model->getFrame()->getName() : "";

    printf("{\"asset\": {\"generator\": \"dff2gltf\", \"version\": \"2.0\"},\n");
    printf("\"scene\": {\"nodes\": [{\"children\": [");

    for (auto const &atomic : model->getAtomics()) {
        printAtomic(atomic, modelName, textures);
        if (&atomic != &model->getAtomics().back()) printf(",");
    }

    printf("]");

    if (!modelName.empty()) {
        printf(", \"name\": \"%s\"", modelName.c_str());
    }

    printf(", \"rotation\": [-0.5,0.5,0.5,0.5]}]}}\n");
}

int main(int argc, char **argv) {
    std::string dff(argv[1]);
    std::string txd(argv[2]);
    try {
        std::string gta3 = getenv("GTA3");
        gta3.erase(std::remove(gta3.begin(), gta3.end(), '\\'), gta3.end());
        auto dirPath = gta3 + "/models/gta3.dir";
        std::vector<DirEntry> assets;
        readfile(dirPath, assets);

        auto imgPath = gta3 + "/models/gta3.img";
        FILE* fp = fopen(imgPath.c_str(), "rb");
        if (!fp) throw "can't open IMG";

        std::vector<char> data;
        auto asset = findentry(assets, dff + ".dff");
        readimg(fp, asset, data);
        auto model = loadDFF(data);

        std::vector<char> txdata;
        if (txd == "generic") {
            readfile(gta3 + "/models/generic.txd", txdata);
        } else {
            auto asset = findentry(assets, txd + ".txd");
            readimg(fp, asset, txdata);
        }
        auto textures = loadTXD(txdata);

        fclose(fp);
        printModel(model, textures);
    } catch (const std::string &s) {
        fprintf(stderr, "Error: %s\n", s.c_str());
    } catch (const char *s) {
        fprintf(stderr, "Error: %s\n", s);
    }
}
