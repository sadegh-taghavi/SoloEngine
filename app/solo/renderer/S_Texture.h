#pragma once

namespace solo
{

class S_TextureSampler;

class S_Texture
{
public:
    S_Texture();
    virtual ~S_Texture();
    const S_TextureSampler *sampler() const;
    void setSampler(const S_TextureSampler *sampler);

private:
    const S_TextureSampler *m_sampler;


};

}

