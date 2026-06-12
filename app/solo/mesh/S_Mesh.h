#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "S_MeshBin.h"
#include "solo/math/S_Math.h"

namespace solo
{

struct S_Joint
{
    int32_t   parent;   // index into joints(), -1 = root
    glm::mat4 invBind;
    glm::vec3 bindT;
    glm::quat bindR;
    glm::vec3 bindS;
};

struct S_AnimChannel
{
    uint32_t joint;
    uint32_t path;        // MeshBinAnimPath
    uint32_t interp;      // MeshBinAnimInterp
    uint32_t keyCount;
    uint32_t timeOffset;  // float index into animKeyData()
    uint32_t valueOffset;
};

struct S_AnimationClip
{
    std::string                name;
    float                      duration = 0.0f;
    std::vector<S_AnimChannel> channels;
};

class S_Mesh
{
public:
    S_Mesh();
    virtual ~S_Mesh();

    uint32_t                         vertexCount()    const;
    uint32_t                         indexCount()     const;
    uint32_t                         primitiveCount() const;
    const std::vector<MeshBinPrimitive>& primitives() const;
    bool                             isSkinned()      const;
    virtual void                     draw()            {}
    virtual uint64_t                 blasAddress() const { return 0; } // RT BLAS, 0 = not built
    virtual uint64_t                 indexAddress() const { return 0; }   // for RT hit shading
    virtual uint64_t                 hitDataAddress() const { return 0; } // for RT hit shading

    // global material table IDs, one per local glTF material index (resolved at load)
    uint32_t globalMaterialID(uint32_t localIndex, uint32_t fallback) const
    {
        return localIndex < m_globalMaterialIDs.size() ? m_globalMaterialIDs[localIndex] : fallback;
    }
    bool hasMaterials() const { return !m_globalMaterialIDs.empty(); }

    const std::vector<MeshBinMaterial>& pendingMaterials() const { return m_pendingMaterials; }
    void setGlobalMaterialIDs(std::vector<uint32_t> ids)
    {
        m_globalMaterialIDs = std::move(ids);
        m_pendingMaterials.clear();
    }

    uint32_t                            skinJointCount() const { return m_skinJointCount; }
    const std::vector<S_Joint>&         joints()         const { return m_joints; }
    const std::vector<uint32_t>&        jointEvalOrder() const { return m_jointEvalOrder; }
    const std::vector<S_AnimationClip>& animations()     const { return m_animations; }
    const std::vector<float>&           animKeyData()    const { return m_animKeyData; }
    int                                 findAnimation(const std::string& name) const;

protected:
    void loadAnimationData(const uint8_t* fileData, const MeshBinHeader& header);

    std::vector<uint32_t>         m_globalMaterialIDs;
    std::vector<MeshBinMaterial>  m_pendingMaterials;
    uint32_t                      m_vertexCount    = 0;
    uint32_t                      m_indexCount     = 0;
    uint32_t                      m_flags          = 0;
    std::vector<MeshBinPrimitive> m_primitives;

    uint32_t                     m_skinJointCount = 0;
    std::vector<S_Joint>         m_joints;
    std::vector<uint32_t>        m_jointEvalOrder;   // parents always before children
    std::vector<S_AnimationClip> m_animations;
    std::vector<float>           m_animKeyData;
};

}
