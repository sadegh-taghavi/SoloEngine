#include "S_Mesh.h"
#include <cstring>

using namespace solo;

S_Mesh::S_Mesh() {}
S_Mesh::~S_Mesh() {}

uint32_t S_Mesh::vertexCount()    const { return m_vertexCount; }
uint32_t S_Mesh::indexCount()     const { return m_indexCount; }
uint32_t S_Mesh::primitiveCount() const { return static_cast<uint32_t>(m_primitives.size()); }
const std::vector<MeshBinPrimitive>& S_Mesh::primitives() const { return m_primitives; }
bool S_Mesh::isSkinned()          const { return (m_flags & MESH_BIN_FLAG_SKINNED) != 0; }

int S_Mesh::findAnimation(const std::string& name) const
{
    for (size_t i = 0; i < m_animations.size(); ++i)
        if (m_animations[i].name == name)
            return static_cast<int>(i);
    return -1;
}

void S_Mesh::loadAnimationData(const uint8_t* fileData, const MeshBinHeader& header)
{
    if (header.version < 2 || header.jointCount == 0)
        return;

    m_skinJointCount = header.skinJointCount;

    m_joints.resize(header.jointCount);
    const MeshBinJoint* srcJoints = reinterpret_cast<const MeshBinJoint*>(fileData + header.jointOffset);
    for (uint32_t j = 0; j < header.jointCount; ++j)
    {
        const MeshBinJoint& src = srcJoints[j];
        S_Joint& dst = m_joints[j];
        dst.parent  = src.parent;
        dst.invBind = glm::make_mat4(src.invBind);
        dst.bindT   = glm::vec3(src.t[0], src.t[1], src.t[2]);
        dst.bindR   = glm::quat(src.r[3], src.r[0], src.r[1], src.r[2]); // glm ctor is (w, x, y, z)
        dst.bindS   = glm::vec3(src.s[0], src.s[1], src.s[2]);
    }

    // evaluation order with parents first (ancestors may be appended after their children)
    m_jointEvalOrder.clear();
    m_jointEvalOrder.reserve(header.jointCount);
    std::vector<bool> placed(header.jointCount, false);
    for (uint32_t added = 0; added < header.jointCount; )
    {
        uint32_t before = added;
        for (uint32_t j = 0; j < header.jointCount; ++j)
        {
            if (placed[j]) continue;
            int p = m_joints[j].parent;
            if (p < 0 || placed[static_cast<uint32_t>(p)])
            {
                m_jointEvalOrder.push_back(j);
                placed[j] = true;
                ++added;
            }
        }
        if (added == before) break; // malformed hierarchy (cycle) — stop rather than loop forever
    }

    const MeshBinAnimation*   srcAnims    = reinterpret_cast<const MeshBinAnimation*>(fileData + header.animationOffset);
    const MeshBinAnimChannel* srcChannels = reinterpret_cast<const MeshBinAnimChannel*>(fileData + header.channelOffset);

    m_animations.resize(header.animationCount);
    for (uint32_t a = 0; a < header.animationCount; ++a)
    {
        const MeshBinAnimation& src = srcAnims[a];
        S_AnimationClip& dst = m_animations[a];

        char name[sizeof(src.name) + 1] = {};
        memcpy(name, src.name, sizeof(src.name));
        dst.name     = name;
        dst.duration = src.duration;

        dst.channels.resize(src.channelCount);
        for (uint32_t c = 0; c < src.channelCount; ++c)
        {
            const MeshBinAnimChannel& sch = srcChannels[src.channelOffset + c];
            dst.channels[c] = { sch.joint, sch.path, sch.interp, sch.keyCount, sch.timeOffset, sch.valueOffset };
        }
    }

    m_animKeyData.resize(header.keyFloatCount);
    if (header.keyFloatCount)
        memcpy(m_animKeyData.data(), fileData + header.keyDataOffset, header.keyFloatCount * sizeof(float));
}
