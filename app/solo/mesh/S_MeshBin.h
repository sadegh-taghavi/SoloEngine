#pragma once
#include <cstdint>

static constexpr uint32_t MESH_BIN_MAGIC   = 0x4853454D;
static constexpr uint32_t MESH_BIN_VERSION = 1;

enum MeshBinFlags : uint32_t
{
    MESH_BIN_FLAG_SKINNED  = 1 << 0,
    MESH_BIN_FLAG_EMISSIVE = 1 << 1,
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

static_assert(sizeof(MeshBinPosition)    == 16, "MeshBinPosition must be 16 bytes");
static_assert(sizeof(MeshBinRasterAttrib)== 20, "MeshBinRasterAttrib must be 20 bytes");
static_assert(sizeof(MeshBinRTHitData)   == 20, "MeshBinRTHitData must be 20 bytes");
static_assert(sizeof(MeshBinSkinVertex)  ==  8, "MeshBinSkinVertex must be 8 bytes");
static_assert(sizeof(MeshBinPrimitive)   == 24, "MeshBinPrimitive must be 24 bytes");
