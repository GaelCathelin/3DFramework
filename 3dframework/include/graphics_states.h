#pragma once

#include <format.h>

typedef enum : uchar {
    BlendFactor_Zero                  = 1,
    BlendFactor_One                   = 2,
    BlendFactor_SrcColor              = 3,
    BlendFactor_InvSrcColor           = 4,
    BlendFactor_SrcAlpha              = 5,
    BlendFactor_InvSrcAlpha           = 6,
    BlendFactor_DstAlpha              = 7,
    BlendFactor_InvDstAlpha           = 8,
    BlendFactor_DstColor              = 9,
    BlendFactor_InvDstColor           = 10,
    BlendFactor_SrcAlphaSaturate      = 11,
    BlendFactor_ConstantColor         = 14,
    BlendFactor_InvConstantColor      = 15,
    BlendFactor_Src1Color             = 16,
    BlendFactor_InvSrc1Color          = 17,
    BlendFactor_Src1Alpha             = 18,
    BlendFactor_InvSrc1Alpha          = 19,
    BlendFactor_OneMinusSrcColor      = BlendFactor_InvSrcColor,
    BlendFactor_OneMinusSrcAlpha      = BlendFactor_InvSrcAlpha,
    BlendFactor_OneMinusDstAlpha      = BlendFactor_InvDstAlpha,
    BlendFactor_OneMinusDstColor      = BlendFactor_InvDstColor,
    BlendFactor_OneMinusConstantColor = BlendFactor_InvConstantColor,
    BlendFactor_OneMinusSrc1Color     = BlendFactor_InvSrc1Color,
    BlendFactor_OneMinusSrc1Alpha     = BlendFactor_InvSrc1Alpha,
} BlendFactor;

typedef enum : uchar {
    BlendOp_Add             = 1,
    BlendOp_Subtract        = 2,
    BlendOp_ReverseSubtract = 3,
    BlendOp_Min             = 4,
    BlendOp_Max             = 5
} BlendOp;

typedef enum : uchar {
    ColorMask_None  = 0,
    ColorMask_Red   = 1,
    ColorMask_Green = 2,
    ColorMask_Blue  = 4,
    ColorMask_Alpha = 8,
    ColorMask_All   = 0xF
} ColorMask;

typedef struct {
    bool        blendEnable;
    BlendFactor srcBlend;
    BlendFactor destBlend;
    BlendOp     blendOp;
    BlendFactor srcBlendAlpha;
    BlendFactor destBlendAlpha;
    BlendOp     blendOpAlpha;
    ColorMask   colorWriteMask;
} RenderTargetBlendState;

typedef struct {
    RenderTargetBlendState targets[8];
    bool alphaToCoverageEnable;
} BlendState;

typedef enum : uchar {
    FillMode_Solid,
    FillMode_Wireframe,
    FillMode_Point,
    FillMode_Fill = FillMode_Solid,
    FillMode_Line = FillMode_Wireframe
} FillMode;

typedef enum : uchar {
    CullMode_Back,
    CullMode_Front,
    CullMode_None
} CullMode;

typedef struct {
    FillMode fillMode;
    CullMode cullMode;
    bool     frontCounterClockwise;
    bool     depthClipEnable;
    bool     scissorEnable;
    bool     sampleShadingEnable;
    bool     rasterizerDiscard;
    bool     antialiasedLineEnable;
    int      depthBias;
    float    depthBiasClamp;
    float    slopeScaledDepthBias;
    uchar    forcedSampleCount; // ?
    bool     programmableSampleLocationsEnable;
    bool     conservativeRasterEnable;
    bool     quadFillEnable; // ?
    char     sampleLocationsX[16]; // 4 bit values
    char     sampleLocationsY[16]; // 4 bit values
} RasterState;

typedef enum : uchar {
    StencilOp_Keep              = 1,
    StencilOp_Zero              = 2,
    StencilOp_Replace           = 3,
    StencilOp_IncrementAndClamp = 4,
    StencilOp_DecrementAndClamp = 5,
    StencilOp_Invert            = 6,
    StencilOp_IncrementAndWrap  = 7,
    StencilOp_DecrementAndWrap  = 8
} StencilOp;

typedef struct {
    StencilOp   failOp;
    StencilOp   depthFailOp;
    StencilOp   passOp;
    CompareFunc stencilFunc;
} StencilOpDesc;

typedef struct {
    bool          depthTestEnable;
    bool          depthWriteEnable;
    CompareFunc   depthFunc;
    bool          stencilEnable;
    uchar         stencilReadMask;
    uchar         stencilWriteMask;
    uchar         stencilRefValue;
    bool          dynamicStencilRef;
    StencilOpDesc frontFaceStencil;
    StencilOpDesc backFaceStencil;
} DepthStencilState;

typedef struct {
    float minX, maxX, minY, maxY, minZ, maxZ;
} ViewportState;

typedef enum : uchar {
    VRS_1x1, VRS_1x2, VRS_2x1, VRS_2x2, VRS_2x4, VRS_4x2, VRS_4x4
} VariableShadingRate;

typedef enum : uchar {
    VRSCombiner_Passthrough,
    VRSCombiner_Override,
    VRSCombiner_Min,
    VRSCombiner_Max,
    VRSCombiner_Mul
} VariableRateShadingCombiner;

typedef struct {
    bool enabled;
    VariableShadingRate shadingRate;
    VariableRateShadingCombiner primitiveCombiner;
    VariableRateShadingCombiner imageCombiner;
} VariableRateShadingState;

typedef struct {
    BlendState               blendState;
    DepthStencilState        depthStencilState;
    RasterState              rasterState;
    ViewportState            viewportState;
    ViewportState            scissorState;
    VariableRateShadingState vrsState;
} RenderState;

#ifdef __cplusplus
extern "C" {
#endif

RenderState* getRenderState();

void pushRenderState();
void popRenderState();
void setDefaultRenderState();
void resetRenderState();

void setBlendFactors(const uint target, const BlendFactor srcBlend, const BlendFactor srcBlendAlpha, const BlendFactor destBlend, const BlendFactor destBlendAlpha);
void setSampleLocation(const uint sample, const float x, const float y);

#ifdef __cplusplus
}
#endif
