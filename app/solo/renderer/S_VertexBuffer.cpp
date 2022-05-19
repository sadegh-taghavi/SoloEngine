#include "S_VertexBuffer.h"

using namespace solo;

S_VertexBuffer::S_VertexBuffer(uint32_t verticesCount, uint32_t indicesCount,
                               uint32_t instancesCount, std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray,
                               std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray):
    m_verticesCount( verticesCount ), m_indicesCount( indicesCount ), m_drawIndicesCount( indicesCount ),
    m_instancesCount( instancesCount ), m_drawInstancesCount( instancesCount ),
    m_verticesDescriptorArray( std::move( verticesDescriptorArray ) ),
    m_instancesDescriptorArray( std::move( instancesDescriptorArray ) )
{

}

S_VertexBuffer::~S_VertexBuffer()
{

}

uint32_t S_VertexBuffer::verticesCount() const
{
    return m_verticesCount;
}

uint32_t S_VertexBuffer::indicesCount() const
{
    return m_indicesCount;
}

uint32_t S_VertexBuffer::instancesCount() const
{
    return m_instancesCount;
}

const S_VertexBufferDescriptorArray *S_VertexBuffer::verticesDescriptorArray() const
{
    return m_verticesDescriptorArray.get();
}

const S_VertexBufferDescriptorArray *S_VertexBuffer::instancesDescriptorArray() const
{
    return m_instancesDescriptorArray.get();
}

uint32_t S_VertexBuffer::drawIndicesCount() const
{
    return m_drawIndicesCount;
}

void S_VertexBuffer::setDrawIndicesCount(const uint32_t &drawIndicesCount)
{
    if( drawIndicesCount < m_indicesCount )
        m_drawIndicesCount = drawIndicesCount;
    else
        m_drawIndicesCount = m_indicesCount;
}

uint32_t S_VertexBuffer::drawInstancesCount() const
{
    return m_drawInstancesCount;
}

void S_VertexBuffer::setDrawInstancesCount(const uint32_t &drawInstancesCount)
{
    if( drawInstancesCount < m_instancesCount )
        m_drawInstancesCount = drawInstancesCount;
    else
        m_drawInstancesCount = drawInstancesCount;
}

S_VertexBufferDescriptorArray::S_VertexBufferDescriptorArray() : m_stride(0)
{

}

S_VertexBufferDescriptorArray::S_VertexBufferDescriptorArray(uint32_t stride, const S_Vector<S_VertexBufferDescriptor> &descriptors) :
    m_stride(stride), m_descriptors( descriptors )
{

}

S_VertexBufferDescriptorArray::~S_VertexBufferDescriptorArray()
{

}

const S_Vector<S_VertexBufferDescriptor> *S_VertexBufferDescriptorArray::descriptors() const
{
    return &m_descriptors;
}

uint32_t S_VertexBufferDescriptorArray::stride() const
{
    return m_stride;
}
