#pragma once
#include <cstdint>

static constexpr uint32_t MESH_BIN_MAGIC   = 0x4853454D;
static constexpr uint32_t MESH_BIN_VERSION = 4;

enum MeshBinFlags : uint32_t
{
    MESH_BIN_FLAG_SKINNED  = 1 << 0,
    MESH_BIN_FLAG_EMISSIVE = 1 << 1,
};

enum MeshBinAnimPath : uint32_t
{
    MESH_BIN_ANIM_TRANSLATION = 0,
    MESH_BIN_ANIM_ROTATION    = 1,
    MESH_BIN_ANIM_SCALE       = 2,
};

enum MeshBinAnimInterp : uint32_t
{
    MESH_BIN_ANIM_LINEAR = 0,
    MESH_BIN_ANIM_STEP   = 1,
};

#pragma pack(push, 1)

struct MeshBinHeader
{
    uint32_t magic;
    uint32_t version;
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t primitiveCount;
    uint32_t flags;
    uint64_t primitiveOffset;
    uint64_t indexOffset;
    uint64_t positionOffset;
    uint64_t rasterAttribOffset;
    uint64_t rtHitDataOffset;
    uint64_t skinOffset;
    uint32_t jointCount;        // total skeleton nodes (skin joints first, then ancestors)
    uint32_t skinJointCount;    // palette size; vertex joint indices are < this
    uint32_t animationCount;
    uint32_t channelCount;      // total channels across all animations
    uint32_t keyFloatCount;     // floats in the keyframe data blob
    uint32_t materialCount;     // v3: MeshBinMaterial records
    uint64_t jointOffset;       // MeshBinJoint[jointCount]
    uint64_t animationOffset;   // MeshBinAnimation[animationCount]
    uint64_t channelOffset;     // MeshBinAnimChannel[channelCount]
    uint64_t keyDataOffset;     // float[keyFloatCount]
    uint64_t materialOffset;    // v3: MeshBinMaterial[materialCount]
};

struct MeshBinMaterial
{
    float baseColorFactor[4];
    float metallicFactor;
    float roughnessFactor;
    char  baseColorPath[64];         // pack-relative KTX paths, empty = none
    char  normalPath[64];
    char  metallicRoughnessPath[64];
};

struct MeshBinJoint
{
    int32_t parent;             // index into joint array, -1 = root (may be > own index)
    float   invBind[16];        // column-major; identity for non-skin ancestor nodes
    float   t[3];               // bind-pose local translation
    float   r[4];               // bind-pose local rotation quaternion (x, y, z, w)
    float   s[3];               // bind-pose local scale
};

struct MeshBinAnimation
{
    char     name[32];
    float    duration;          // seconds
    uint32_t channelOffset;     // first channel index into channel array
    uint32_t channelCount;
};

struct MeshBinAnimChannel
{
    uint32_t joint;             // index into joint array
    uint32_t path;              // MeshBinAnimPath
    uint32_t interp;            // MeshBinAnimInterp
    uint32_t keyCount;
    uint32_t timeOffset;        // float index into key data blob (keyCount floats)
    uint32_t valueOffset;       // float index into key data blob (keyCount * 3 or 4 floats)
};

struct MeshBinPrimitive
{
    uint32_t indexOffset;
    uint32_t indexCount;
    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t materialID;
    uint32_t pad;
};

struct MeshBinPosition
{
    float x, y, z, pad;
};

struct MeshBinRasterAttrib
{
    float   uv[2];
    int8_t  normal[3];
    int8_t  tangentSign;
    int8_t  tangent[3];
    int8_t  pad;
    uint8_t color[4];
};

struct MeshBinRTHitData
{
    float    uv[2];
    int8_t   normal[3];
    int8_t   pad0;
    int8_t   tangent[3];
    int8_t   pad1;
    uint32_t materialID;
};

struct MeshBinSkinVertex
{
    uint8_t joints[4];
    uint8_t weights[4];
};

#pragma pack(pop)

static_assert(sizeof(MeshBinJoint)       == 108, "MeshBinJoint must be 108 bytes");
static_assert(sizeof(MeshBinAnimation)   == 44,  "MeshBinAnimation must be 44 bytes");
static_assert(sizeof(MeshBinAnimChannel) == 24,  "MeshBinAnimChannel must be 24 bytes");
static_assert(sizeof(MeshBinPosition)    == 16, "MeshBinPosition must be 16 bytes");
static_assert(sizeof(MeshBinRasterAttrib)== 20, "MeshBinRasterAttrib must be 20 bytes");
static_assert(sizeof(MeshBinRTHitData)   == 20, "MeshBinRTHitData must be 20 bytes");
static_assert(sizeof(MeshBinSkinVertex)  ==  8, "MeshBinSkinVertex must be 8 bytes");
static_assert(sizeof(MeshBinPrimitive)   == 24, "MeshBinPrimitive must be 24 bytes");
