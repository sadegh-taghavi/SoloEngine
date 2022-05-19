#include "S_Texture.h"

using namespace solo;

S_Texture::S_Texture(): m_sampler(nullptr)
{

}

S_Texture::~S_Texture()
{

}

const S_TextureSampler *S_Texture::sampler() const
{
    return m_sampler;
}

void S_Texture::setSampler(const S_TextureSampler *sampler)
{
    m_sampler = sampler;
}
