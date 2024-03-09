#pragma once

#include <mesh.h>

DECL_OPAQUE_TYPE(AccelerationStructure)

typedef struct {
    uint first, count;
} Range;

#ifdef __cplusplus
extern "C" {
#endif

AccelerationStructure createAccelerationStructure(const Mesh mesh, const bool updatable) WARN_UNUSED_RESULT;
AccelerationStructure createMultiAccelerationStructure(const Mesh mesh, const Range *ranges, const uint nbRanges, const bool updatable) WARN_UNUSED_RESULT;
void updateAccelerationStructure(AccelerationStructure as, const Mesh mesh, const Range *ranges, const uint nbRanges);
void deleteAccelerationStructure(AccelerationStructure *as);

#ifdef __cplusplus
}
#endif
