#include "S_Animator.h"
#include "solo/mesh/S_Mesh.h"
#include <algorithm>

using namespace solo;

S_Animator::S_Animator(S_Mesh* mesh) : m_mesh(mesh)
{
    const size_t jointCount = mesh ? mesh->joints().size() : 0;
    m_localT.resize(jointCount);
    m_localR.resize(jointCount);
    m_localS.resize(jointCount);
    m_globals.resize(jointCount);
    m_palette.resize(mesh ? mesh->skinJointCount() : 0, glm::mat4(1.0f));

    if (mesh && !mesh->animations().empty())
        setClip(0u);
    else
        computePalette(); // bind pose
}

bool S_Animator::setClip(const std::string& name)
{
    int idx = m_mesh ? m_mesh->findAnimation(name) : -1;
    return idx >= 0 && setClip(static_cast<uint32_t>(idx));
}

bool S_Animator::setClip(uint32_t index)
{
    if (!m_mesh || index >= m_mesh->animations().size())
        return false;
    m_clip = static_cast<int>(index);
    m_time = 0.0f;
    computePalette();
    return true;
}

float S_Animator::duration() const
{
    if (!m_mesh || m_clip < 0)
        return 0.0f;
    return m_mesh->animations()[static_cast<size_t>(m_clip)].duration;
}

void S_Animator::setTime(float seconds)
{
    m_time = seconds;
    computePalette();
}

void S_Animator::update(float dtSeconds)
{
    if (!m_mesh || m_clip < 0)
        return;

    const float dur = duration();
    m_time += dtSeconds * m_speed;
    if (dur > 0.0f)
    {
        if (m_loop)
            m_time = std::fmod(m_time, dur) + (m_time < 0.0f ? dur : 0.0f);
        else
            m_time = glm::clamp(m_time, 0.0f, dur);
    }
    computePalette();
}

void S_Animator::computePalette()
{
    if (!m_mesh)
        return;

    const auto& joints = m_mesh->joints();
    if (joints.empty())
        return;

    for (size_t j = 0; j < joints.size(); ++j)
    {
        m_localT[j] = joints[j].bindT;
        m_localR[j] = joints[j].bindR;
        m_localS[j] = joints[j].bindS;
    }

    if (m_clip >= 0)
    {
        const S_AnimationClip& clip = m_mesh->animations()[static_cast<size_t>(m_clip)];
        const float* keys = m_mesh->animKeyData().data();

        for (const S_AnimChannel& ch : clip.channels)
        {
            const float* times = keys + ch.timeOffset;
            const uint32_t comps = (ch.path == MESH_BIN_ANIM_ROTATION) ? 4u : 3u;

            // keyframe pair around m_time, and the interpolation factor between them
            uint32_t k1 = static_cast<uint32_t>(
                std::upper_bound(times, times + ch.keyCount, m_time) - times);
            if (k1 >= ch.keyCount) k1 = ch.keyCount - 1u;
            const uint32_t k0 = (k1 > 0u) ? k1 - 1u : 0u;

            float f = 0.0f;
            if (k1 != k0 && ch.interp == MESH_BIN_ANIM_LINEAR)
            {
                const float t0 = times[k0], t1 = times[k1];
                f = (t1 > t0) ? glm::clamp((m_time - t0) / (t1 - t0), 0.0f, 1.0f) : 0.0f;
            }

            const float* v0 = keys + ch.valueOffset + k0 * comps;
            const float* v1 = keys + ch.valueOffset + k1 * comps;

            switch (ch.path)
            {
            case MESH_BIN_ANIM_TRANSLATION:
                m_localT[ch.joint] = glm::mix(glm::vec3(v0[0], v0[1], v0[2]),
                                              glm::vec3(v1[0], v1[1], v1[2]), f);
                break;
            case MESH_BIN_ANIM_ROTATION:
            {
                const glm::quat q0(v0[3], v0[0], v0[1], v0[2]);
                const glm::quat q1(v1[3], v1[0], v1[1], v1[2]);
                m_localR[ch.joint] = glm::slerp(q0, q1, f);
                break;
            }
            case MESH_BIN_ANIM_SCALE:
                m_localS[ch.joint] = glm::mix(glm::vec3(v0[0], v0[1], v0[2]),
                                              glm::vec3(v1[0], v1[1], v1[2]), f);
                break;
            }
        }
    }

    for (uint32_t j : m_mesh->jointEvalOrder())
    {
        glm::mat4 local = glm::translate(glm::mat4(1.0f), m_localT[j])
                        * glm::mat4_cast(m_localR[j])
                        * glm::scale(glm::mat4(1.0f), m_localS[j]);
        const int p = joints[j].parent;
        m_globals[j] = (p >= 0) ? m_globals[static_cast<uint32_t>(p)] * local : local;
    }

    for (uint32_t j = 0; j < m_mesh->skinJointCount(); ++j)
        m_palette[j] = m_globals[j] * joints[j].invBind;
}
