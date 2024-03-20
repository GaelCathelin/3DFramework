#include <mesh.h>
#include "private_impl.h"
#include "private_log.h"

extern "C" {

Mesh createMesh(PrimitiveType type) {
    MeshImpl *mesh = new MeshImpl();
    mesh->primitiveType = type;
    return Mesh{mesh};
}

void deleteMesh(Mesh *mesh) {
    if (!mesh || !mesh->impl)
        return;

    MeshImpl *impl = getMesh(*mesh);
    deleteBuffer(&impl->indices);
    for (uint i = 0; i < impl->buffers.size(); i++) {
        deleteBuffer(&impl->buffers[i]);
    }
    delete impl;
    mesh->impl = nullptr;
}

void setMeshIndices(Mesh mesh, const uint *indices, const uint nbIndices) {
    MeshImpl *impl = getMesh(mesh);
    impl->nbIndices = nbIndices;
    impl->indicesFormat = nvrhi::Format::R32_UINT;

    deleteBuffer(&impl->indices);
    if (nbIndices > 0) {
        impl->indices = createBuffer(ResourceType_IndexBuffer, nbIndices * sizeof(uint));
        if (indices) setBufferData(impl->indices, indices, nbIndices * sizeof(uint));
    }
}

void setMeshIndices16(Mesh mesh, const ushort *indices, const uint nbIndices) {
    MeshImpl *impl = getMesh(mesh);
    impl->nbIndices = nbIndices;
    impl->indicesFormat = nvrhi::Format::R16_UINT;

    deleteBuffer(&impl->indices);
    if (nbIndices > 0) {
        impl->indices = createBuffer(ResourceType_IndexBuffer, nbIndices * sizeof(ushort));
        if (indices) setBufferData(impl->indices, indices, nbIndices * sizeof(ushort));
    }
}

void addMeshAttrib(Mesh mesh, const Format format, const bool instanced, const void *data, const uint nbElems) {
    MeshImpl *impl = getMesh(mesh);
    impl->nbVertices = nbElems;
    const uint elemSize = getFormatInfo(format).size;

    impl->attributes.push_back(nvrhi::VertexAttributeDesc()
        .setFormat((nvrhi::Format)format).setElementStride(elemSize)
        .setIsInstanced(instanced)
        .setBufferIndex(impl->attributes.size()));

    Buffer buffer = createBuffer(ResourceType_VertexBuffer, nbElems * elemSize);
    if (data) setBufferData(buffer, data, nbElems * elemSize);
    impl->buffers.push_back(buffer);
}

Buffer getIndexBuffer(Mesh mesh) {return getMesh(mesh)->indices;}
Buffer getAttribBuffer(Mesh mesh, const uint i) {return getMesh(mesh)->buffers.at(i);}
uint getNbIndices(Mesh mesh) {return getMesh(mesh)->nbIndices;}
uint getNbVertices(Mesh mesh) {return getMesh(mesh)->nbVertices;}

Mesh createUnitCubePatch() {
    const Float4 vertices[8] = {
        {-1, -1, -1}, { 1, -1, -1}, {-1,  1, -1}, { 1,  1, -1},
        {-1, -1,  1}, { 1, -1,  1}, {-1,  1,  1}, { 1,  1,  1},
    };
    const uint indices[6][4] = {
        {0, 1, 2, 3}, {4, 5, 0, 1}, {5, 7, 1, 3},
        {7, 6, 3, 2}, {6, 4, 2, 0}, {6, 7, 4, 5},
    };

    Mesh cube = createMesh(PrimitiveType_Patches);
    setMeshIndices(cube, (uint*)indices, 24);
    addMeshAttrib(cube, RGBA32_FLOAT, false, vertices, 8);
    return cube;
}

}

MeshImpl* getMesh(Mesh mesh) {return (MeshImpl*)mesh.impl;}
