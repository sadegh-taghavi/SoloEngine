#pragma once
#include "solo/platforms/S_BaseApplication.h"
#include "solo/renderer/S_Renderer.h"
#include "solo/renderer/S_Scene.h"
#include "solo/renderer/S_Animator.h"
#include "solo/ui/S_UI.h"
#include "solo/pack/S_Pack.h"
#include <chrono>
#include <memory>

namespace solo
{

class S_Application : public S_BaseApplication
{
public:
    S_Application( unsigned int width, unsigned int height );
    virtual void onCreateEvent();
    virtual void onSpinEvent();
    virtual void onResizeEvent(const S_WindowResizeEvent *event);
    virtual void onFocusEvent(const S_WindowFocusEvent *event);
    S_Renderer *renderer() const;
    S_Pack *pack() const;
    static S_Application *executingApplication();

private:
    char __padding[4];
    std::unique_ptr<S_Renderer> m_renderer;
    std::unique_ptr<S_Pack>     m_pack;
    S_VertexBufferHandle m_vVB;
    S_VertexBufferHandle m_vGround;
    S_ShaderHandle       m_vShader;
    S_TextureHandle      m_vTexture;
    S_SamplerHandle      m_vSampler;
    S_ShaderHandle       m_meshShader;
    S_ShaderHandle       m_skinnedShader;
    S_Scene              m_scene;
    S_MeshHandle         m_foxMesh;
    std::unique_ptr<S_Animator> m_foxAnimator;
    uint32_t             m_foxNode = 0;
    std::unique_ptr<S_UI> m_ui;
    glm::vec2            m_foxPos     = { 8.0f, 0.0f }; // x, z on the ground plane
    float                m_foxHeading = 0.0f;
    std::shared_ptr<class S_CameraPerspective> m_vCam;
    std::shared_ptr<class S_FirstPersonCameraController> m_vCamController;
    std::chrono::steady_clock::time_point m_startTime;

};

}
