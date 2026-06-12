#pragma once
#include "solo/platforms/S_BaseApplication.h"
#include "solo/renderer/S_Renderer.h"
#include "solo/scene/S_EntityScene.h"
#include "solo/renderer/S_Animator.h"
#include "solo/ui/S_UI.h"
#include "solo/physics/S_Physics.h"
#include "solo/audio/S_Audio.h"
#include "solo/pack/S_Pack.h"
#include <vector>
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
    S_ShaderHandle       m_meshShader;
    S_ShaderHandle       m_skinnedShader;
    S_EntityScene        m_scene;
    S_MeshHandle         m_foxMesh;
    std::unique_ptr<S_Animator> m_foxAnimator;
    S_EntityID           m_fox = S_NO_ENTITY;
    std::unique_ptr<S_UI> m_ui;
    float                m_foxHeading = 0.0f;
    bool                 m_followCam  = true;
    bool                 m_rtShadows  = true;
    std::unique_ptr<S_Physics>   m_physics;
    std::unique_ptr<S_Character> m_foxCharacter;
    std::unique_ptr<S_Audio>   m_audio;
    S_MeshHandle              m_boxMesh;
    uint32_t                  m_boxMat    = 0;
    uint32_t                  m_groundMat = 0;
    int                       m_spawnCounter = 0;
    std::shared_ptr<class S_CameraPerspective> m_vCam;
    std::shared_ptr<class S_FirstPersonCameraController> m_vCamController;
    std::chrono::steady_clock::time_point m_startTime;

};

}
