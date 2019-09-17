// https://github.com/rwengine/openrw/blob/master/rwcore/loaders/LoaderDFF.cpp
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <numeric>

#include <boost/range/adaptors.hpp>
#include <glm/glm.hpp>

#include "Clump.hpp"
#include "RWBinaryStream.hpp"

#include "util.h"

FrameList readFrameList(const RWBStream& stream);

GeometryList readGeometryList(const RWBStream& stream);

GeometryPtr readGeometry(const RWBStream& stream);

void readMaterialList(const GeometryPtr& geom, const RWBStream& stream);

void readMaterial(const GeometryPtr& geom, const RWBStream& stream);

void readTexture(Geometry::Material& material, const RWBStream& stream);

void readGeometryExtension(const GeometryPtr& geom, const RWBStream& stream);

void readBinMeshPLG(const GeometryPtr& geom, const RWBStream& stream);

AtomicPtr readAtomic(FrameList& framelist, GeometryList& geometrylist,
                        const RWBStream& stream);

enum DFFChunks {
    CHUNK_STRUCT = 0x0001,
    CHUNK_EXTENSION = 0x0003,
    CHUNK_TEXTURE = 0x0006,
    CHUNK_MATERIAL = 0x0007,
    CHUNK_MATERIALLIST = 0x0008,
    CHUNK_FRAMELIST = 0x000E,
    CHUNK_GEOMETRY = 0x000F,
    CHUNK_CLUMP = 0x0010,

    CHUNK_ATOMIC = 0x0014,

    CHUNK_GEOMETRYLIST = 0x001A,

    CHUNK_BINMESHPLG = 0x050E,

    CHUNK_NODENAME = 0x0253F2FE,
};

// These structs are used to interpret raw bytes from the stream.
/// @todo worry about endianness.

typedef glm::vec3 BSTVector3;
typedef glm::mat3 BSTMatrix;
typedef glm::i8vec4 BSTColour;

struct RWBSFrame {
    BSTMatrix rotation;
    BSTVector3 position;
    int32_t index;
    uint32_t matrixflags;  // Not used
};

FrameList readFrameList(const RWBStream &stream) {
    auto listStream = stream.getInnerStream();

    auto listStructID = listStream.getNextChunk();
    if (listStructID != CHUNK_STRUCT) {
        throw "Frame List missing struct chunk";
    }

    char *headerPtr = listStream.getCursor();

    unsigned int numFrames = *reinterpret_cast<std::uint32_t *>(headerPtr);
    headerPtr += sizeof(std::uint32_t);

    FrameList framelist;
    framelist.reserve(numFrames);

    for (auto f = 0u; f < numFrames; ++f) {
        auto data = reinterpret_cast<RWBSFrame *>(headerPtr);
        headerPtr += sizeof(RWBSFrame);
        auto frame =
            std::make_shared<ModelFrame>(f, data->rotation, data->position);

        assert(data->index < static_cast<int>(framelist.size()) &&
                 "Frame parent out of bounds");
        if (data->index != -1 &&
            data->index < static_cast<int>(framelist.size())) {
            framelist[data->index]->addChild(frame);
        }

        framelist.push_back(frame);
    }

    size_t namedFrames = 0;

    /// @todo perhaps flatten this out a little
    for (auto chunkID = listStream.getNextChunk(); chunkID != 0;
         chunkID = listStream.getNextChunk()) {
        switch (chunkID) {
            case CHUNK_EXTENSION: {
                auto extStream = listStream.getInnerStream();
                for (auto chunkID = extStream.getNextChunk(); chunkID != 0;
                     chunkID = extStream.getNextChunk()) {
                    switch (chunkID) {
                        case CHUNK_NODENAME: {
                            std::string fname(extStream.getCursor(),
                                              extStream.getCurrentChunkSize());
                            std::transform(fname.begin(), fname.end(),
                                           fname.begin(), ::tolower);

                            if (namedFrames < framelist.size()) {
                                framelist[namedFrames++]->setName(fname);
                            }
                        } break;
                        default:
                            break;
                    }
                }
            } break;
            default:
                break;
        }
    }

    return framelist;
}

GeometryList readGeometryList(const RWBStream &stream) {
    auto listStream = stream.getInnerStream();

    auto listStructID = listStream.getNextChunk();
    if (listStructID != CHUNK_STRUCT) {
        throw "Geometry List missing struct chunk";
    }

    char *headerPtr = listStream.getCursor();

    unsigned int numGeometries = bit_cast<std::uint32_t>(*headerPtr);
    headerPtr += sizeof(std::uint32_t);

    std::vector<GeometryPtr> geometrylist;
    geometrylist.reserve(numGeometries);

    for (auto chunkID = listStream.getNextChunk(); chunkID != 0;
         chunkID = listStream.getNextChunk()) {
        switch (chunkID) {
            case CHUNK_GEOMETRY: {
                geometrylist.push_back(readGeometry(listStream));
            } break;
            default:
                break;
        }
    }

    return geometrylist;
}

GeometryPtr readGeometry(const RWBStream &stream) {
    auto geomStream = stream.getInnerStream();

    auto geomStructID = geomStream.getNextChunk();
    if (geomStructID != CHUNK_STRUCT) {
        throw "Geometry missing struct chunk";
    }

    auto geom = std::make_shared<Geometry>();

    char *headerPtr = geomStream.getCursor();

    geom->flags = bit_cast<std::uint16_t>(*headerPtr);
    headerPtr += sizeof(std::uint16_t);

    /*unsigned short numUVs = bit_cast<std::uint8_t>(*headerPtr);*/
    headerPtr += sizeof(std::uint8_t);
    /*unsigned short moreFlags = bit_cast<std::uint8_t>(*headerPtr);*/
    headerPtr += sizeof(std::uint8_t);

    unsigned int numTris = bit_cast<std::uint32_t>(*headerPtr);
    headerPtr += sizeof(std::uint32_t);
    unsigned int numVerts = bit_cast<std::uint32_t>(*headerPtr);
    headerPtr += sizeof(std::uint32_t);

    /*unsigned int numFrames = bit_cast<std::uint32_t>(*headerPtr);*/
    headerPtr += sizeof(std::uint32_t);

    geom->verts.resize(numVerts);

    if (geomStream.getChunkVersion() < 0x1003FFFF) {
        headerPtr += sizeof(RW::BSGeometryColor);
    }

    /// @todo extract magic numbers.

    if ((geom->flags & 8) == 8) {
        for (size_t v = 0; v < numVerts; ++v) {
            geom->verts[v].colour = bit_cast<glm::u8vec4>(*headerPtr);
            headerPtr += sizeof(glm::u8vec4);
        }
    } else {
        for (size_t v = 0; v < numVerts; ++v) {
            geom->verts[v].colour = {255, 255, 255, 255};
        }
    }

    if ((geom->flags & 4) == 4 || (geom->flags & 128) == 128) {
        for (size_t v = 0; v < numVerts; ++v) {
            geom->verts[v].texcoord = bit_cast<glm::vec2>(*headerPtr);
            headerPtr += sizeof(glm::vec2);
        }
    }

    // Grab indicies data to generate normals (if applicable).
    auto triangles = std::make_unique<RW::BSGeometryTriangle[]>(numTris);
    memcpy(triangles.get(), headerPtr, sizeof(RW::BSGeometryTriangle) * numTris);
    headerPtr += sizeof(RW::BSGeometryTriangle) * numTris;

    geom->geometryBounds = bit_cast<RW::BSGeometryBounds>(*headerPtr);
    geom->geometryBounds.radius = std::abs(geom->geometryBounds.radius);
    headerPtr += sizeof(RW::BSGeometryBounds);

    for (size_t v = 0; v < numVerts; ++v) {
        geom->verts[v].position = bit_cast<glm::vec3>(*headerPtr);
        headerPtr += sizeof(glm::vec3);
    }

    if ((geom->flags & 16) == 16) {
        for (size_t v = 0; v < numVerts; ++v) {
            geom->verts[v].normal = bit_cast<glm::vec3>(*headerPtr);
            headerPtr += sizeof(glm::vec3);
        }
    } else {
        // Use triangle data to calculate normals for each vert.
        for (size_t t = 0; t < numTris; ++t) {
            auto &triangle = triangles[t];
            auto &A = geom->verts[triangle.first];
            auto &B = geom->verts[triangle.second];
            auto &C = geom->verts[triangle.third];
            auto normal = glm::normalize(
                glm::cross(C.position - A.position, B.position - A.position));
            A.normal = normal;
            B.normal = normal;
            C.normal = normal;
        }
    }

    // Process the geometry child sections
    for (auto chunkID = geomStream.getNextChunk(); chunkID != 0;
         chunkID = geomStream.getNextChunk()) {
        switch (chunkID) {
            case CHUNK_MATERIALLIST:
                readMaterialList(geom, geomStream);
                break;
            case CHUNK_EXTENSION:
                readGeometryExtension(geom, geomStream);
                break;
            default:
                break;
        }
    }

    return geom;
}

void readMaterialList(const GeometryPtr &geom, const RWBStream &stream) {
    auto listStream = stream.getInnerStream();

    auto listStructID = listStream.getNextChunk();
    if (listStructID != CHUNK_STRUCT) {
        throw "MaterialList missing struct chunk";
    }

    unsigned int numMaterials = bit_cast<std::uint32_t>(*listStream.getCursor());

    geom->materials.reserve(numMaterials);

    RWBStream::ChunkID chunkID;
    while ((chunkID = listStream.getNextChunk())) {
        switch (chunkID) {
            case CHUNK_MATERIAL:
                readMaterial(geom, listStream);
                break;
            default:
                break;
        }
    }
}

void readMaterial(const GeometryPtr &geom, const RWBStream &stream) {
    auto materialStream = stream.getInnerStream();

    auto matStructID = materialStream.getNextChunk();
    if (matStructID != CHUNK_STRUCT) {
        throw "Material missing struct chunk";
    }

    char *matData = materialStream.getCursor();

    Geometry::Material material;

    // Unkown
    matData += sizeof(std::uint32_t);
    material.colour = bit_cast<glm::u8vec4>(*matData);

    matData += sizeof(std::uint32_t);
    // Unkown
    matData += sizeof(std::uint32_t);
    /*bool usesTexture = bit_cast<std::uint32_t>(*matData);*/
    matData += sizeof(std::uint32_t);

    material.ambientIntensity = bit_cast<float>(*matData);
    matData += sizeof(float);

    /*float specular = bit_cast<float>(*matData);*/
    matData += sizeof(float);

    material.diffuseIntensity = bit_cast<float>(*matData);
    matData += sizeof(float);
    material.flags = 0;

    RWBStream::ChunkID chunkID;
    while ((chunkID = materialStream.getNextChunk())) {
        switch (chunkID) {
            case CHUNK_TEXTURE:
                readTexture(material, materialStream);
                break;
            default:
                break;
        }
    }

    geom->materials.push_back(material);
}

void readTexture(Geometry::Material &material,
                            const RWBStream &stream) {
    auto texStream = stream.getInnerStream();

    auto texStructID = texStream.getNextChunk();
    if (texStructID != CHUNK_STRUCT) {
        throw "Texture missing struct chunk";
    }

    // There's some data in the Texture's struct, but we don't know what it is.

    /// @todo improve how these strings are read.
    std::string name, alpha;

    texStream.getNextChunk();
    name = texStream.getCursor();
    texStream.getNextChunk();
    alpha = texStream.getCursor();

    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    std::transform(alpha.begin(), alpha.end(), alpha.begin(), ::tolower);

    material.textures.emplace_back(std::move(name), std::move(alpha));
}

void readGeometryExtension(const GeometryPtr &geom,
                                      const RWBStream &stream) {
    auto extStream = stream.getInnerStream();

    RWBStream::ChunkID chunkID;
    while ((chunkID = extStream.getNextChunk())) {
        switch (chunkID) {
            case CHUNK_BINMESHPLG:
                readBinMeshPLG(geom, extStream);
                break;
            default:
                break;
        }
    }
}

void readBinMeshPLG(const GeometryPtr &geom, const RWBStream &stream) {
    auto data = stream.getCursor();

    geom->facetype = static_cast<Geometry::FaceType>(bit_cast<std::uint32_t>(*data));
    data += sizeof(std::uint32_t);

    unsigned int numSplits = bit_cast<std::uint32_t>(*data);
    data += sizeof(std::uint32_t);

    // Number of triangles.
    data += sizeof(std::uint32_t);

    geom->subgeom.reserve(numSplits);

    size_t start = 0;

    for (size_t s = 0; s < numSplits; ++s) {
        SubGeometry sg;
        sg.numIndices = bit_cast<std::uint32_t>(*data);
        data += sizeof(std::uint32_t);
        sg.material = bit_cast<std::uint32_t>(*data);
        data += sizeof(std::uint32_t);
        sg.start = start;
        start += sg.numIndices;

        sg.indices.resize(sg.numIndices);
        std::memcpy(sg.indices.data(), data,
                    sizeof(std::uint32_t) * sg.numIndices);
        data += sizeof(std::uint32_t) * sg.numIndices;

        geom->subgeom.push_back(std::move(sg));
    }
}

AtomicPtr readAtomic(FrameList &framelist,
                                GeometryList &geometrylist,
                                const RWBStream &stream) {
    auto atomicStream = stream.getInnerStream();

    auto atomicStructID = atomicStream.getNextChunk();
    if (atomicStructID != CHUNK_STRUCT) {
        throw "Atomic missing struct chunk";
    }

    auto data = atomicStream.getCursor();
    std::uint32_t frame = bit_cast<std::uint32_t>(*data);
    data += sizeof(std::uint32_t);

    std::uint32_t geometry = bit_cast<std::uint32_t>(*data);
    data += sizeof(std::uint32_t);

    std::uint32_t flags = bit_cast<std::uint32_t>(*data);

    // Verify the atomic's particulars
    assert(frame < framelist.size() && "atomic frame out of bounds");
    assert(geometry < geometrylist.size() &&
             "atomic geometry out of bounds");

    auto atomic = std::make_shared<Atomic>();
    if (geometry < geometrylist.size()) {
        atomic->setGeometry(geometrylist[geometry]);
    }
    if (frame < framelist.size()) {
        atomic->setFrame(framelist[frame]);
    }
    atomic->setFlags(flags);

    return atomic;
}

void gltf(AtomicPtr atomic) {
    auto &geom = atomic->getGeometry();
    auto &name = atomic->getFrame()->getName();

    GLenum mode = geom->facetype == Geometry::Triangles ? GL_TRIANGLES : GL_TRIANGLE_STRIP;

    printf("{\"asset\": {\"generator\": \"dff2gltf\", \"version\": \"2.0\"},\n");
    printf("\"buffers\": [");
    size_t attrLength = sizeof(GeometryVertex) * geom->verts.size();
    printf("{\"uri\": \"%s.attributes.bin\", \"byteLength\": %lu}, ",
            name.c_str(), attrLength);
    FILE* bin = fopen((name + ".attributes.bin").c_str(), "wb");
    fwrite(geom->verts.data(), sizeof(GeometryVertex), geom->verts.size(), bin);
    fclose(bin);

    size_t icount = std::accumulate(
        geom->subgeom.begin(), geom->subgeom.end(), size_t{0u},
        [](size_t a, const SubGeometry &b) { return a + b.numIndices; });
    printf("{\"uri\": \"%s.indices.bin\", \"byteLength\": %lu}",
            name.c_str(), sizeof(uint32_t) * icount);
    printf("],\n");

    printf("\"bufferViews\":\n");
    printf("[{\"buffer\": 0, \"target\": %d, \"byteLength\": %lu, \"byteStride\": %lu}\n",
            GL_ARRAY_BUFFER, attrLength, sizeof(GeometryVertex));

    bin = fopen((name + ".indices.bin").c_str(), "wb");
    for (auto &sg : geom->subgeom) {
        printf(",{\"buffer\": 1, \"target\": %d, \"byteOffset\": %lu, \"byteLength\": %lu}\n",
                GL_ELEMENT_ARRAY_BUFFER, sg.start * sizeof(uint32_t), sizeof(uint32_t) * sg.numIndices);
        fwrite(sg.indices.data(), sizeof(uint32_t), sg.numIndices, bin);
    }
    fclose(bin);
    printf("],\n");

    glm::vec3 minPos = std::accumulate(
        std::next(geom->verts.begin()), geom->verts.end(), geom->verts[0].position,
        [](glm::vec3 a, const GeometryVertex &v) { return glm::min(a, v.position); });
    glm::vec3 maxPos = std::accumulate(
        std::next(geom->verts.begin()), geom->verts.end(), geom->verts[0].position,
        [](glm::vec3 a, const GeometryVertex &v) { return glm::max(a, v.position); });

    printf("\"accessors\":\n[");
    for (auto &attr : GeometryVertex::vertex_attributes()) {
        if (attr.offset) printf(",");
        printf("{\"bufferView\": 0, \"type\": \"VEC%d\", \"byteOffset\": %lu, \"componentType\": %d, \"count\": %lu",
                attr.size, attr.offset, attr.type, geom->verts.size());
        if (attr.sem == ATRS_Position) {
            printf(", \"min\": [%.9g,%.9g,%.9g]", minPos.x, minPos.y, minPos.z);
            printf(", \"max\": [%.9g,%.9g,%.9g]", maxPos.x, maxPos.y, maxPos.z);
        } else if (attr.sem == ATRS_Colour) {
            printf(", \"normalized\": true");
        }
        printf("}\n");
    }
    for (auto const &sg : geom->subgeom | boost::adaptors::indexed(1)) {
        printf(",{\"bufferView\": %lu, \"type\": \"SCALAR\", \"componentType\": %d, \"count\": %lu}\n",
                sg.index(), GL_UNSIGNED_INT, sg.value().numIndices);
    }
    printf("],\n");

    printf("\"images\":\n[");
    for (auto &mat : geom->materials) {
        printf("{\"uri\": \"txd/%s.png\"}\n", mat.textures[0].name.c_str());
        if (&mat != &geom->materials.back()) printf(",");
    }
    printf("],\n");

    printf("\"samplers\":\n[");
    for (auto &mat : geom->materials) {
        FILE* meta = fopen(("txd/" + mat.textures[0].name + ".json").c_str(), "r");
        char buf[1024];
        while (size_t buflen = fread(buf, 1, sizeof(buf), meta)) {
            fwrite(buf, 1, buflen, stdout);
        }
        fclose(meta);
        if (&mat != &geom->materials.back()) printf(",");
    }
    printf("],\n");

    printf("\"textures\":\n[");
    for (int i = 0; i < geom->materials.size(); i++) {
        if (i) printf(",");
        printf("{\"source\": %d, \"sampler\": %d}\n", i, i);
    }
    printf("],\n");

    printf("\"materials\":\n[");
    for (auto const &mat : geom->materials | boost::adaptors::indexed(0)) {
        if (mat.index()) printf(",");
        printf("{\"pbrMetallicRoughness\": {\"baseColorTexture\": {\"index\": %lu}}, ", mat.index());
        printf("\"doubleSided\": true, \"alphaMode\": \"");
        FILE* meta = fopen(("txd/" + mat.value().textures[0].name + ".txt").c_str(), "r");
        char buf[1024];
        while (size_t buflen = fread(buf, 1, sizeof(buf), meta)) {
            fwrite(buf, 1, buflen, stdout);
        }
        fclose(meta);
        printf("\"}\n");
    }
    printf("],\n");

    printf("\"meshes\": [{\"primitives\":\n[");
    const char *attrobj = "\"attributes\": {\"POSITION\": 0, \"NORMAL\": 1, \"TEXCOORD_0\": 2, \"COLOR_0\": 3}";
    for (auto const &sg : geom->subgeom | boost::adaptors::indexed(4)) {
        printf("{\"mode\": %d, \"indices\": %lu, \"material\": %lu, %s}\n",
                mode, sg.index(), sg.value().material, attrobj);
        if (&sg.value() != &geom->subgeom.back()) printf(",");
    }
    printf("]}],\n");

    printf("\"nodes\": [{\"mesh\": 0, \"rotation\": [-0.5,0.5,0.5,0.5]}],\n");
    printf("\"scenes\": [{\"nodes\": [0]}],\n");
    printf("\"scene\": 0}\n");
}

int main(int argc, char **argv) {
    std::string fname(argv[1]);

    std::vector<char> data;
    readfile(fname, data);

    auto model = std::make_shared<Clump>();

    RWBStream rootStream(data.data(), data.size());

    auto rootID = rootStream.getNextChunk();
    if (rootID != CHUNK_CLUMP) {
        throw "Invalid root section ID " + std::to_string(rootID);
    }

    RWBStream modelStream = rootStream.getInnerStream();
    auto rootStructID = modelStream.getNextChunk();
    if (rootStructID != CHUNK_STRUCT) {
        throw "Clump missing struct chunk";
    }

    // There is only one value in the struct section.
    std::uint32_t numAtomics = bit_cast<std::uint32_t>(*rootStream.getCursor());
    (void)numAtomics;

    GeometryList geometrylist;
    FrameList framelist;

    // Process everything inside the clump stream.
    RWBStream::ChunkID chunkID;
    while ((chunkID = modelStream.getNextChunk())) {
        switch (chunkID) {
            case CHUNK_FRAMELIST:
                framelist = readFrameList(modelStream);
                break;
            case CHUNK_GEOMETRYLIST:
                geometrylist = readGeometryList(modelStream);
                break;
            case CHUNK_ATOMIC: {
                auto atomic = readAtomic(framelist, geometrylist, modelStream);
                assert(atomic && "Failed to read atomic");
                if (!atomic) {
                    // Abort reading the rest of the clump
                    return 1;
                }
                model->addAtomic(atomic);
                gltf(atomic);
            } break;
            default:
                break;
        }
    }

    if (!framelist.empty()) {
        model->setFrame(framelist[0]);
    }

    // Ensure the model has cached metrics
    model->recalculateMetrics();
}
