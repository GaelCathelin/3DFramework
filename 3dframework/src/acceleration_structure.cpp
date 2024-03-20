#include <acceleration_structure.h>
#include "private_impl.h"
#include "private_log.h"

static nvrhi::rt::AccelStructDesc getBlasDesc(MeshImpl *meshimpl, const Range *ranges, const uint nbRanges, const nvrhi::rt::AccelStructBuildFlags flags) {
    nvrhi::rt::GeometryTriangles trianglesBase = nvrhi::rt::GeometryTriangles()
        .setVertexBuffer(getNvBuffer(meshimpl->buffers[0]))
        .setVertexFormat(meshimpl->attributes[0].format).setVertexStride(getFormatInfo((Format)meshimpl->attributes[0].format).size)
        .setVertexOffset(0).setVertexCount(meshimpl->nbVertices);

    if (meshimpl->indices.impl)
        trianglesBase.setIndexBuffer(getNvBuffer(meshimpl->indices)).setIndexFormat(nvrhi::Format::R32_UINT);

    nvrhi::rt::AccelStructDesc blasDesc = nvrhi::rt::AccelStructDesc().setIsTopLevel(false).setBuildFlags(flags);
    for (uint i = 0; i < nbRanges; i++) {
        nvrhi::rt::GeometryTriangles triangles = trianglesBase;
        if (meshimpl->indices.impl)
            triangles.setIndexOffset(ranges[i].first * 4).setIndexCount(ranges[i].count);
        else
            triangles.setVertexOffset(ranges[i].first * trianglesBase.vertexStride).setVertexCount(ranges[i].count);

        blasDesc.addBottomLevelGeometry(nvrhi::rt::GeometryDesc().setTriangles(triangles).setFlags(nvrhi::rt::GeometryFlags::Opaque));
    }

    return blasDesc;
}

extern "C" {

AccelerationStructure createAccelerationStructure(const Mesh mesh, const bool updatable) {
    Range r = {0, getIndexBuffer(mesh).impl ? getNbIndices(mesh) : getNbVertices(mesh)};
    return createMultiAccelerationStructure(mesh, &r, 1, updatable);
}

AccelerationStructure createMultiAccelerationStructure(const Mesh mesh, const Range *ranges, const uint nbRanges, const bool updatable) {
    if (!raytracingEnabled()) {
        logError("Creation of an acceleration structure while raytracing is disabled.");
        return AccelerationStructure{nullptr};
    }

    AccelerationStructureImpl *as = new AccelerationStructureImpl();
    nvrhi::rt::AccelStructBuildFlags flags = nvrhi::rt::AccelStructBuildFlags::AllowDataAccess | (updatable ?
        (nvrhi::rt::AccelStructBuildFlags::PreferFastBuild | nvrhi::rt::AccelStructBuildFlags::AllowUpdate) :
        (nvrhi::rt::AccelStructBuildFlags::PreferFastTrace));

    nvrhi::rt::AccelStructDesc blasDesc = getBlasDesc(getMesh(mesh), ranges, nbRanges, flags);
    as->blas = getDevice()->createAccelStruct(blasDesc);
    getCommandList()->buildBottomLevelAccelStruct(as->blas, blasDesc.bottomLevelGeometries.data(), blasDesc.bottomLevelGeometries.size(), flags);

    nvrhi::rt::AccelStructDesc tlasDesc = nvrhi::rt::AccelStructDesc().setIsTopLevel(true).setTopLevelMaxInstances(1).setBuildFlags(flags);
    as->tlas = getDevice()->createAccelStruct(tlasDesc);

    nvrhi::rt::InstanceDesc instanceDesc = nvrhi::rt::InstanceDesc().setBLAS(as->blas).setInstanceMask(0xFF)
        .setFlags(nvrhi::rt::InstanceFlags::ForceOpaque | nvrhi::rt::InstanceFlags::TriangleCullDisable);

    getCommandList()->buildTopLevelAccelStruct(as->tlas, &instanceDesc, 1, flags);

    return AccelerationStructure{as};
}

void updateAccelerationStructure(AccelerationStructure as, const Mesh mesh, const Range *ranges, const uint nbRanges) {
    nvrhi::rt::AccelStructBuildFlags flags = nvrhi::rt::AccelStructBuildFlags::AllowDataAccess | nvrhi::rt::AccelStructBuildFlags::PreferFastBuild | nvrhi::rt::AccelStructBuildFlags::AllowUpdate;
    nvrhi::rt::AccelStructDesc blasDesc = getBlasDesc(getMesh(mesh), ranges, nbRanges, flags);
    getCommandList()->buildBottomLevelAccelStruct(getAccelerationStructure(as)->blas, blasDesc.bottomLevelGeometries.data(), blasDesc.bottomLevelGeometries.size(), flags);

    nvrhi::rt::InstanceDesc instanceDesc = nvrhi::rt::InstanceDesc().setBLAS(getAccelerationStructure(as)->blas).setInstanceMask(0xFF)
        .setFlags(nvrhi::rt::InstanceFlags::ForceOpaque | nvrhi::rt::InstanceFlags::TriangleCullDisable);

    getCommandList()->buildTopLevelAccelStruct(getAccelerationStructure(as)->tlas, &instanceDesc, 1, flags | nvrhi::rt::AccelStructBuildFlags::PerformUpdate);
}

void deleteAccelerationStructure(AccelerationStructure *as) {
    if (!as || as->impl)
        return;

    AccelerationStructureImpl *impl = getAccelerationStructure(*as);
    impl->tlas.Reset();
    impl->blas.Reset();
    delete impl;
    as->impl = nullptr;
}

}

AccelerationStructureImpl* getAccelerationStructure(AccelerationStructure as) {return (AccelerationStructureImpl*)as.impl;}
