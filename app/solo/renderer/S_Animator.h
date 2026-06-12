#pragma once
#include <vector>
#include <string>
#include "solo/math/S_Math.h"

namespace solo
{

class S_Mesh;

// Samples a skinned mesh's animation clips on the CPU and produces the joint
// matrix palette consumed by the skinned vertex shader.
class S_Animator
{
public:
    explicit S_Animator(S_Mesh* mesh);

    bool  setClip(const std::string& name);
    bool  setClip(uint32_t index);
    int   clip()     const { return m_clip; }
    float duration() const;

    void  setLooping(bool loop)   { m_loop = loop; }
    void  setSpeed(float speed)   { m_speed = speed; }
    void  setTime(float seconds);
    float time() const            { return m_time; }

    void update(float dtSeconds);

    const std::vector<glm::mat4>& palette() const { return m_palette; }

private:
    void computePalette();

    S_Mesh* m_mesh;
    int     m_clip  = -1;
    float   m_time  = 0.0f;
    float   m_speed = 1.0f;
    bool    m_loop  = true;

    std::vector<glm::vec3> m_localT;
    std::vector<glm::quat> m_localR;
    std::vector<glm::vec3> m_localS;
    std::vector<glm::mat4> m_globals;
    std::vector<glm::mat4> m_palette;
};

}
