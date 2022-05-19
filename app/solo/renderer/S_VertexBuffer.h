#pragma once

#include "solo/math/S_Vec4.h"
#include "solo/math/S_Vec3.h"
#include "solo/math/S_Vec2.h"
#include "solo/stl/S_Vector.h"
#include <memory>

namespace solo
{

enum class S_VertexBufferDescriptorFormat
{
    UNDEFINED = 0,
    R8_UNORM = 9,
    R8_SNORM = 10,
    R8_USCALED = 11,
    R8_SSCALED = 12,
    R8_UINT = 13,
    R8_SINT = 14,
    R8G8_UNORM = 16,
    R8G8_SNORM = 17,
    R8G8_USCALED = 18,
    R8G8_SSCALED = 19,
    R8G8_UINT = 20,
    R8G8_SINT = 21,
    R8G8B8_UNORM = 23,
    R8G8B8_SNORM = 24,
    R8G8B8_USCALED = 25,
    R8G8B8_SSCALED = 26,
    R8G8B8_UINT = 27,
    R8G8B8_SINT = 28,
    B8G8R8_UNORM = 30,
    B8G8R8_SNORM = 31,
    B8G8R8_USCALED = 32,
    B8G8R8_SSCALED = 33,
    B8G8R8_UINT = 34,
    B8G8R8_SINT = 35,
    R8G8B8A8_UNORM = 37,
    R8G8B8A8_SNORM = 38,
    R8G8B8A8_USCALED = 39,
    R8G8B8A8_SSCALED = 40,
    R8G8B8A8_UINT = 41,
    R8G8B8A8_SINT = 42,
    B8G8R8A8_UNORM = 44,
    B8G8R8A8_SNORM = 45,
    B8G8R8A8_USCALED = 46,
    B8G8R8A8_SSCALED = 47,
    B8G8R8A8_UINT = 48,
    B8G8R8A8_SINT = 49,
    R16_UNORM = 70,
    R16_SNORM = 71,
    R16_USCALED = 72,
    R16_SSCALED = 73,
    R16_UINT = 74,
    R16_SINT = 75,
    R16_SFLOAT = 76,
    R16G16_UNORM = 77,
    R16G16_SNORM = 78,
    R16G16_USCALED = 79,
    R16G16_SSCALED = 80,
    R16G16_UINT = 81,
    R16G16_SINT = 82,
    R16G16_SFLOAT = 83,
    R16G16B16_UNORM = 84,
    R16G16B16_SNORM = 85,
    R16G16B16_USCALED = 86,
    R16G16B16_SSCALED = 87,
    R16G16B16_UINT = 88,
    R16G16B16_SINT = 89,
    R16G16B16_SFLOAT = 90,
    R16G16B16A16_UNORM = 91,
    R16G16B16A16_SNORM = 92,
    R16G16B16A16_USCALED = 93,
    R16G16B16A16_SSCALED = 94,
    R16G16B16A16_UINT = 95,
    R16G16B16A16_SINT = 96,
    R16G16B16A16_SFLOAT = 97,
    R32_UINT = 98,
    R32_SINT = 99,
    R32_SFLOAT = 100,
    R32G32_UINT = 101,
    R32G32_SINT = 102,
    R32G32_SFLOAT = 103,
    R32G32B32_UINT = 104,
    R32G32B32_SINT = 105,
    R32G32B32_SFLOAT = 106,
    R32G32B32A32_UINT = 107,
    R32G32B32A32_SINT = 108,
    R32G32B32A32_SFLOAT = 109,
    R64_UINT = 110,
    R64_SINT = 111,
    R64_SFLOAT = 112,
    R64G64_UINT = 113,
    R64G64_SINT = 114,
    R64G64_SFLOAT = 115,
    R64G64B64_UINT = 116,
    R64G64B64_SINT = 117,
    R64G64B64_SFLOAT = 118,
    R64G64B64A64_UINT = 119,
    R64G64B64A64_SINT = 120,
    R64G64B64A64_SFLOAT = 121
};

struct S_VertexBufferDescriptor
{
    uint32_t Size;
    uint32_t Offset;
    S_VertexBufferDescriptorFormat Format;
};

class S_VertexBufferDescriptorArray
{
public:
    S_VertexBufferDescriptorArray();
    S_VertexBufferDescriptorArray(uint32_t stride, const S_Vector<S_VertexBufferDescriptor> &descriptors );
    ~S_VertexBufferDescriptorArray();
    const S_Vector<S_VertexBufferDescriptor> *descriptors() const;
    uint32_t stride() const;  

private:
    uint32_t m_stride;
    S_Vector<S_VertexBufferDescriptor> m_descriptors;
};

class S_VertexBuffer
{
public:
    S_VertexBuffer(uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                   std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray, std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray);
    virtual ~S_VertexBuffer();
    virtual std::pair<void *, void *> beginVerticesData() = 0;
    virtual void endVerticesData() = 0;
    virtual void * beginInstancesData() = 0;
    virtual void endInstancesData() = 0;
    virtual void draw() = 0;
    uint32_t verticesCount() const;
    uint32_t indicesCount() const;
    uint32_t instancesCount() const;
    const S_VertexBufferDescriptorArray *verticesDescriptorArray() const;
    const S_VertexBufferDescriptorArray *instancesDescriptorArray() const;
    uint32_t drawIndicesCount() const;
    void setDrawIndicesCount(const uint32_t &drawIndicesCount);
    uint32_t drawInstancesCount() const;
    void setDrawInstancesCount(const uint32_t &drawInstancesCount);

protected:
    uint32_t m_verticesCount;
    uint32_t m_indicesCount;
    uint32_t m_drawIndicesCount;
    uint32_t m_instancesCount;
    uint32_t m_drawInstancesCount;
    std::unique_ptr<S_VertexBufferDescriptorArray> m_verticesDescriptorArray;
    std::unique_ptr<S_VertexBufferDescriptorArray> m_instancesDescriptorArray;
};

}

