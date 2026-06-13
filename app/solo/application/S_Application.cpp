#include "S_Application.h"
#include "solo/debug/S_Debug.h"
#include "solo/math/S_Math.h"
#include "solo/renderer/S_RendererAPI.h"
#include "solo/renderer/S_Shader.h"
#include "solo/renderer/S_Texture.h"
#include "solo/mesh/S_Mesh.h"
#include "solo/renderer/S_PerFrame.h"
#include "solo/renderer/S_Camera.h"
#include "solo/renderer/S_CameraController.h"
#include "solo/renderer/vulkan/S_VulkanRendererAPI.h"
#include <imgui.h>
#include <list>

using namespace solo;

solo::S_Application::S_Application(unsigned int width, unsigned int height) : S_BaseApplication ( width, height )
{

}

void solo::S_Application::onCreateEvent()
{
    s_debugLayer( "onCreateEvent" );
    m_startTime = std::chrono::steady_clock::now();
    m_pack = std::make_unique<S_Pack>();
    m_pack->open("resources.spk");
    m_renderer = std::make_unique<S_Renderer>();

    m_meshShader    = m_renderer->createShader("shaders/mesh", "shaders/mesh", "", "");
    m_skinnedShader = m_renderer->createShader("shaders/skinned", "shaders/mesh", "", "");
    m_renderer->createMeshPipelines(m_meshShader, m_skinnedShader);

    S_MeshHandle shotgunMesh = m_renderer->createMesh("models/scene.mesh.bin");
    S_EntityID shotgun = m_scene.create("shotgun");
    m_scene.attachMesh(shotgun, shotgunMesh);
    m_scene.setPosition(shotgun, glm::vec3(0.0f, 1.0f, 0.0f));
    m_scene.setScale(shotgun, glm::vec3(10.0f));

    m_foxMesh     = m_renderer->createMesh("models/Fox.mesh.bin");
    m_foxAnimator = std::make_unique<S_Animator>(m_renderer->getMesh(m_foxMesh));
    if (!m_foxAnimator->setClip("Run"))
        m_foxAnimator->setClip(0u);

    m_fox = m_scene.create("fox");
    m_scene.attachMesh(m_fox, m_foxMesh);
    m_scene.attachAnimator(m_fox, m_foxAnimator.get());
    m_scene.setPosition(m_fox, glm::vec3(8.0f, 0.0f, 0.0f));
    m_scene.setScale(m_fox, glm::vec3(0.05f));

    m_ui = std::make_unique<S_UI>(static_cast<S_VulkanRendererAPI*>(m_renderer->api()),
                                  "fonts/Roboto-Regular.ui.bin");

    inputMap()->load("input/bindings.json");

    m_physics = std::make_unique<S_Physics>();
    // static ground slab; top face flush with the visual ground quad at y = -0.65
    m_physics->createBox(glm::vec3(22.0f, 0.5f, 22.0f), glm::vec3(0.0f, -1.15f, 0.0f), S_BodyType::Static);
    m_boxMesh   = m_renderer->createMesh("models/Box.mesh.bin");
    m_boxMat    = m_renderer->createMaterial(glm::vec4(0.85f, 0.55f, 0.30f, 1.0f), "", 0.1f, 0.7f); // crate orange
    m_groundMat = m_renderer->createMaterial(glm::vec4(0.45f, 0.62f, 0.38f, 1.0f), "", 0.30f, 0.16f); // glossy green floor

    S_EntityID ground = m_scene.create("ground");
    m_scene.attachMesh(ground, m_boxMesh, m_groundMat);
    m_scene.setPosition(ground, glm::vec3(0.0f, -1.15f, 0.0f));
    m_scene.setScale(ground, glm::vec3(44.0f, 1.0f, 44.0f));

    // PBR calibration sheet: metallic rows (bottom 0 → top 1),
    // roughness columns (left 0.05 → right 1.0), near-white base color
    S_MeshHandle sphereMesh = m_renderer->createMesh("models/Sphere.mesh.bin");
    for (int row = 0; row < 5; ++row)
        for (int col = 0; col < 5; ++col)
        {
            const float metallic  = static_cast<float>(row) / 4.0f;
            const float roughness = 0.05f + 0.95f * static_cast<float>(col) / 4.0f;
            const uint32_t mat = m_renderer->createMaterial(
                glm::vec4(0.95f, 0.93f, 0.90f, 1.0f), "", metallic, roughness);
            S_EntityID s = m_scene.create("sheet_m" + std::to_string(row) + "_r" + std::to_string(col));
            m_scene.attachMesh(s, sphereMesh, mat);
            m_scene.setPosition(s, glm::vec3(8.0f + (col - 2) * 2.2f,
                                             2.0f + row * 2.2f, -8.0f));
            m_scene.setScale(s, glm::vec3(1.8f));
        }

    m_foxCharacter = m_physics->createCharacter(0.6f, 0.6f, glm::vec3(8.0f, 0.0f, 0.0f));

    m_audio = std::make_unique<S_Audio>();

    m_vCam = std::make_shared<S_CameraPerspective>();
    m_vCam->setPosition(glm::vec3(0.f, 28.f, 40.f));
    m_vCamController = std::make_shared<S_FirstPersonCameraController>();
    m_vCamController->setCamera(m_vCam);

    m_renderer->setRenderCallback([this]()
    {
        const float elapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - m_startTime).count();
        const float frameDt = static_cast<float>(m_renderer->elapsedTimeUs()) / 1000000.0f;

        m_vCam->setWidth(static_cast<float>(window()->width()));
        m_vCam->setHeight(static_cast<float>(window()->height()));
        if (m_followCam)
        {
            const glm::vec3 foxPos = m_foxCharacter->position();
            m_vCam->setPosition(foxPos + glm::vec3(0.0f, 6.0f, 12.0f));
            m_vCam->setTarget(foxPos + glm::vec3(0.0f, 1.5f, 0.0f));
            m_vCam->update();
        }
        else
            m_vCamController->update(frameDt);

        m_audio->setListener(m_vCam->position(),
                             glm::normalize(m_vCam->target() - m_vCam->position()));
        m_audio->update();

        {
            S_PerFrameData pfd;
            pfd.VP        = m_vCam->viewProjection();
            pfd.time      = elapsed;
            pfd.rtShadows = m_rtShadows ? 1.0f : 0.0f;
            pfd.cameraPos = glm::vec4(m_vCam->position(), 0.0f);
            m_renderer->updatePerFrame(pfd);
        }

        {
            ImGui::Begin("Solo Debug");
            ImGui::Text("%.1f fps (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Text("audio voices: %u", m_audio->activeVoices());
            if (ImGui::Checkbox("Follow camera", &m_followCam) && !m_followCam)
                m_vCamController->setCamera(m_vCam); // adopt current pose for free-fly
            ImGui::Checkbox("RT shadows", &m_rtShadows);
            ImGui::SeparatorText("Fox animation");

            auto* foxMesh = m_renderer->getMesh(m_foxMesh);
            if (foxMesh && !foxMesh->animations().empty())
            {
                int clip = m_foxAnimator->clip();
                if (ImGui::BeginCombo("Clip", clip >= 0 ? foxMesh->animations()[clip].name.c_str() : "<none>"))
                {
                    for (int i = 0; i < static_cast<int>(foxMesh->animations().size()); ++i)
                        if (ImGui::Selectable(foxMesh->animations()[i].name.c_str(), i == clip))
                            m_foxAnimator->setClip(static_cast<uint32_t>(i));
                    ImGui::EndCombo();
                }
                static float speed = 1.0f;
                if (ImGui::SliderFloat("Speed", &speed, 0.0f, 3.0f))
                    m_foxAnimator->setSpeed(speed);
                ImGui::Text("time %.2f / %.2f s", m_foxAnimator->time(), m_foxAnimator->duration());
            }
            ImGui::End();
        }

        // native UI demo: anchored bottom panel, SDF text, animated 9-slice button
        // all sizes in logical units * display scale, so high-DPI keeps physical size
        const float uiScale = window()->scaleFactor();
        m_ui->setScale(uiScale);
        {
            auto* foxMesh = m_renderer->getMesh(m_foxMesh);
            glm::vec4 r = m_ui->anchoredRect(0.5f, 1.0f, 0.0f, -95.0f * uiScale,
                                             430.0f * uiScale, 150.0f * uiScale);
            m_ui->panel(r.x, r.y, r.z, r.w);
            m_ui->text("SOLO ENGINE", r.x + r.z * 0.5f, r.y + 16.0f * uiScale, 28.0f * uiScale,
                       S_UI::rgba(235, 240, 255), true);

            if (foxMesh && !foxMesh->animations().empty())
            {
                int  clip = m_foxAnimator->clip();
                int  next = (clip + 1) % static_cast<int>(foxMesh->animations().size());
                if (m_ui->button("nextClip", "Play: " + foxMesh->animations()[next].name,
                                 r.x + r.z * 0.5f - 215.0f * uiScale, r.y + 62.0f * uiScale,
                                 205.0f * uiScale, 56.0f * uiScale))
                {
                    m_audio->play("sounds/click.wav");
                    m_foxAnimator->setClip(static_cast<uint32_t>(next));
                }
            }

            if (m_ui->button("dropBoxes", "Drop boxes",
                             r.x + r.z * 0.5f + 10.0f * uiScale, r.y + 62.0f * uiScale,
                             205.0f * uiScale, 56.0f * uiScale))
            {
                m_audio->play("sounds/click.wav");
                for (int i = 0; i < 8; ++i)
                {
                    ++m_spawnCounter;
                    const float fx = static_cast<float>((m_spawnCounter * 37) % 13) - 6.0f;
                    const float fz = static_cast<float>((m_spawnCounter * 53) % 9)  - 4.0f;
                    const float fy = 14.0f + static_cast<float>(i) * 2.2f;
                    glm::quat tilt = glm::angleAxis(0.4f * static_cast<float>(m_spawnCounter % 7),
                                                    glm::normalize(glm::vec3(0.3f, 1.0f, 0.7f)));
                    S_BodyHandle body = m_physics->createBox(glm::vec3(0.75f), glm::vec3(fx, fy, fz),
                                                             S_BodyType::Dynamic, tilt);
                    S_EntityID box = m_scene.create("box" + std::to_string(m_spawnCounter));
                    m_scene.attachMesh(box, m_boxMesh, m_boxMat);
                    m_scene.attachBody(box, body);
                    m_scene.setScale(box, glm::vec3(1.5f)); // unit cube * 2*halfExtent
                }
            }
        }

        // virtual joystick feeds the action map; the fox is a Jolt character:
        // capsule collision vs ground and boxes, gravity, jumping (Space / Pad.A)
        {
            glm::vec4 jr = m_ui->anchoredRect(0.0f, 1.0f, 130.0f * uiScale, -130.0f * uiScale, 0.0f, 0.0f);
            glm::vec2 stick = m_ui->joystick("move", jr.x, jr.y, 85.0f * uiScale);
            inputMap()->setVirtualAxis("MoveX", stick.x);
            inputMap()->setVirtualAxis("MoveY", -stick.y); // stick up = forward

            const float mx = inputMap()->axis("MoveX");
            const float my = inputMap()->axis("MoveY");
            const float speed = 8.0f;
            glm::vec3 desired(mx * speed, 0.0f, -my * speed); // forward = -Z

            m_foxCharacter->update(frameDt, desired, inputMap()->actionPressed("Jump"));

            if (m_foxCharacter->justLanded())
                m_audio->play("sounds/thud.wav", m_foxCharacter->position(), 0.35f);

            const glm::vec3 vel = m_foxCharacter->velocity();
            const bool moving = glm::length(glm::vec2(vel.x, vel.z)) > 0.5f;
            if (moving)
            {
                m_foxHeading = std::atan2(vel.x, vel.z);
                if (m_foxAnimator->clip() != m_renderer->getMesh(m_foxMesh)->findAnimation("Run"))
                    m_foxAnimator->setClip("Run");
            }
            else if (m_foxAnimator->clip() == m_renderer->getMesh(m_foxMesh)->findAnimation("Run"))
                m_foxAnimator->setClip("Survey");

            m_scene.setPosition(m_fox, m_foxCharacter->position());
            m_scene.setRotation(m_fox, glm::angleAxis(m_foxHeading, glm::vec3(0.0f, 1.0f, 0.0f)));
        }

        m_physics->update(frameDt);
        m_scene.update(m_physics.get(), frameDt); // animators + body sync + world transforms

        // impact thuds straight from the contact listener (box-ground and box-box)
        for (const S_ContactEvent& contact : m_physics->contactEvents())
            if (contact.impactSpeed > 2.0f)
                m_audio->play("sounds/thud.wav", contact.position,
                              glm::clamp(contact.impactSpeed / 18.0f, 0.15f, 1.0f));

        m_renderer->clearDraws();
        m_scene.render(*m_renderer);
        m_renderer->flushDraws(m_meshShader, m_skinnedShader);
    });

    S_BaseApplication::onCreateEvent();
}

void solo::S_Application::onSpinEvent()
{
    m_renderer->drawFrame();
    S_BaseApplication::onSpinEvent();
}

void S_Application::onResizeEvent(const S_WindowResizeEvent *event)
{
    s_debugLayer( "onResizeEvent", event->width(), event->height() );
    m_renderer->resize( event->width(), event->height() );
    S_BaseApplication::onResizeEvent( event );
}

void S_Application::onFocusEvent(const S_WindowFocusEvent *event)
{
    s_debugLayer( "onFocusEvent", event->focus() );
    m_renderer->active( event->focus() );
    S_BaseApplication::onFocusEvent( event );
}

S_Renderer* S_Application::renderer() const
{
    return m_renderer.get();
}

S_Pack *S_Application::pack() const
{
    return m_pack.get();
}

S_Application *S_Application::executingApplication()
{
    return static_cast<S_Application *>( S_BaseApplication::executingApplication() );
}
