#pragma once

#include <buffer.h>
#include <format.h>

#define NullMesh (Mesh){0}

DECL_OPAQUE_TYPE(Mesh)

typedef enum : uchar {
    PrimitiveType_Points,
    PrimitiveType_Lines,
    PrimitiveType_LineStrip,
    PrimitiveType_Triangles,
    PrimitiveType_TriangleStrip,
    PrimitiveType_TriangleFan,
    PrimitiveType_TrianglesWithAdjacency,
    PrimitiveType_TriangleStripWithAdjacency,
    PrimitiveType_Patches,
} PrimitiveType;

#ifdef __cplusplus
extern "C" {
#endif

Mesh createMesh(PrimitiveType type) WARN_UNUSED_RESULT;
void deleteMesh(Mesh *mesh);

void setMeshIndices(Mesh mesh, const uint *indices, const uint nbIndices);
void setMeshIndices16(Mesh mesh, const ushort *indices, const uint nbIndices);
void addMeshAttrib(Mesh mesh, const Format format, const bool instanced, const void *data, const uint nbElems);

Buffer getIndexBuffer(Mesh mesh);
Buffer getAttribBuffer(Mesh mesh, const uint i);
uint getNbIndices(Mesh mesh);
uint getNbVertices(Mesh mesh);

Mesh createUnitCubePatch() WARN_UNUSED_RESULT;

#ifdef __cplusplus
}
#endif
