#include "S_Application.h"
#include "solo/debug/S_Debug.h"
#include "solo/math/S_Math.h"
#include "solo/renderer/S_RendererAPI.h"
#include "solo/renderer/S_Shader.h"
#include "solo/renderer/S_Texture.h"
#include "solo/mesh/S_Mesh.h"
#include "solo/renderer/S_Scene.h"
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

    struct Vertex { glm::vec3 position; glm::vec2 texcoord; };
    struct Instance { glm::vec4 transform; glm::vec4 color; };

    std::vector<S_VertexBufferDescriptor> vertexDescs(2);
    vertexDescs[0].Size   = static_cast<uint32_t>(sizeof(glm::vec3));
    vertexDescs[0].Offset = static_cast<uint32_t>(offsetof(Vertex, position));
    vertexDescs[0].Format = S_VertexBufferDescriptorFormat::R32G32B32_SFLOAT;
    vertexDescs[1].Size   = static_cast<uint32_t>(sizeof(glm::vec2));
    vertexDescs[1].Offset = static_cast<uint32_t>(offsetof(Vertex, texcoord));
    vertexDescs[1].Format = S_VertexBufferDescriptorFormat::R32G32_SFLOAT;

    std::vector<S_VertexBufferDescriptor> instanceDescs(2);
    instanceDescs[0].Size   = static_cast<uint32_t>(sizeof(glm::vec4));
    instanceDescs[0].Offset = static_cast<uint32_t>(offsetof(Instance, transform));
    instanceDescs[0].Format = S_VertexBufferDescriptorFormat::R32G32B32A32_SFLOAT;
    instanceDescs[1].Size   = static_cast<uint32_t>(sizeof(glm::vec4));
    instanceDescs[1].Offset = static_cast<uint32_t>(offsetof(Instance, color));
    instanceDescs[1].Format = S_VertexBufferDescriptorFormat::R32G32B32A32_SFLOAT;

    m_vShader = m_renderer->createShader("shaders/vs", "shaders/ps", "", "");

    S_PipelineDescriptor pd;
    pd.VertexBufferDescriptorArray   = S_VertexBufferDescriptorArray(static_cast<uint32_t>(sizeof(Vertex)),   vertexDescs);
    pd.InstanceBufferDescriptorArray = S_VertexBufferDescriptorArray(static_cast<uint32_t>(sizeof(Instance)), instanceDescs);
    pd.Shader = m_renderer->getShader(m_vShader);

    m_meshShader = m_renderer->createShader("shaders/mesh", "shaders/mesh", "", "");

    std::vector<S_VertexBufferDescriptor> meshPosDescs(1);
    meshPosDescs[0].Size   = static_cast<uint32_t>(sizeof(float) * 3);
    meshPosDescs[0].Offset = 0;
    meshPosDescs[0].Format = S_VertexBufferDescriptorFormat::R32G32B32_SFLOAT;

    S_PipelineDescriptor meshPd;
    meshPd.VertexBufferDescriptorArray   = S_VertexBufferDescriptorArray(static_cast<uint32_t>(sizeof(MeshBinPosition)), meshPosDescs);
    meshPd.InstanceBufferDescriptorArray = S_VertexBufferDescriptorArray();
    meshPd.Shader           = m_renderer->getShader(m_meshShader);
    meshPd.UseEngineGlobals = true;

    m_skinnedShader = m_renderer->createShader("shaders/skinned", "shaders/mesh", "", "");

    std::vector<S_VertexBufferDescriptor> skinDescs(2);
    skinDescs[0].Size   = static_cast<uint32_t>(sizeof(uint8_t) * 4);
    skinDescs[0].Offset = static_cast<uint32_t>(offsetof(MeshBinSkinVertex, joints));
    skinDescs[0].Format = S_VertexBufferDescriptorFormat::R8G8B8A8_UINT;
    skinDescs[1].Size   = static_cast<uint32_t>(sizeof(uint8_t) * 4);
    skinDescs[1].Offset = static_cast<uint32_t>(offsetof(MeshBinSkinVertex, weights));
    skinDescs[1].Format = S_VertexBufferDescriptorFormat::R8G8B8A8_UNORM;

    S_PipelineDescriptor skinnedPd;
    skinnedPd.VertexBufferDescriptorArray = S_VertexBufferDescriptorArray(static_cast<uint32_t>(sizeof(MeshBinPosition)), meshPosDescs);
    skinnedPd.SkinBufferDescriptorArray   = S_VertexBufferDescriptorArray(static_cast<uint32_t>(sizeof(MeshBinSkinVertex)), skinDescs);
    skinnedPd.Shader           = m_renderer->getShader(m_skinnedShader);
    skinnedPd.UseEngineGlobals = true;

    m_renderer->createGraphicsPipeline({ pd, meshPd, skinnedPd });

    S_MeshHandle shotgun    = m_renderer->createMesh("models/scene.mesh.bin");
    uint32_t     shotgunMat = m_renderer->createMaterial();
    glm::mat4 shotgunTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f))
                               * glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
    m_scene.addNode(shotgun, shotgunTransform, shotgunMat);

    m_foxMesh           = m_renderer->createMesh("models/Fox.mesh.bin");
    uint32_t     foxMat = m_renderer->createMaterial();
    glm::mat4 foxTransform = glm::translate(glm::mat4(1.0f), glm::vec3(8.0f, 0.0f, 0.0f))
                           * glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));
    m_foxNode = m_scene.addNode(m_foxMesh, foxTransform, foxMat);

    m_foxAnimator = std::make_unique<S_Animator>(m_renderer->getMesh(m_foxMesh));
    if (!m_foxAnimator->setClip("Run"))
        m_foxAnimator->setClip(0u);

    m_ui = std::make_unique<S_UI>(static_cast<S_VulkanRendererAPI*>(m_renderer->api()),
                                  "fonts/Roboto-Regular.ui.bin");

    m_vVB = m_renderer->createVertexBuffer(24, 36, 1600,
        std::make_unique<S_VertexBufferDescriptorArray>(static_cast<uint32_t>(sizeof(Vertex)),   vertexDescs),
        std::make_unique<S_VertexBufferDescriptorArray>(static_cast<uint32_t>(sizeof(Instance)), instanceDescs));

    {
        auto* vb  = m_renderer->getVertexBuffer(m_vVB);
        auto  vbr = vb->beginVerticesData();
        Vertex   *v   = reinterpret_cast<Vertex *>(vbr.first);
        uint32_t *idx = reinterpret_cast<uint32_t *>(vbr.second);

        constexpr float h = 0.4f;

        v[ 0]={{ -h, 1.f, -h },{ 0.f, 0.f }};
        v[ 1]={{ -h, 1.f,  h },{ 1.f, 0.f }};
        v[ 2]={{  h, 1.f, -h },{ 0.f, 1.f }};
        v[ 3]={{  h, 1.f,  h },{ 1.f, 1.f }};

        v[ 4]={{ -h, 0.f, -h },{ 0.f, 0.f }};
        v[ 5]={{  h, 0.f, -h },{ 1.f, 0.f }};
        v[ 6]={{ -h, 0.f,  h },{ 0.f, 1.f }};
        v[ 7]={{  h, 0.f,  h },{ 1.f, 1.f }};

        v[ 8]={{ -h, 0.f,  h },{ 0.f, 0.f }};
        v[ 9]={{  h, 0.f,  h },{ 1.f, 0.f }};
        v[10]={{ -h, 1.f,  h },{ 0.f, 1.f }};
        v[11]={{  h, 1.f,  h },{ 1.f, 1.f }};

        v[12]={{  h, 0.f, -h },{ 0.f, 0.f }};
        v[13]={{ -h, 0.f, -h },{ 1.f, 0.f }};
        v[14]={{  h, 1.f, -h },{ 0.f, 1.f }};
        v[15]={{ -h, 1.f, -h },{ 1.f, 1.f }};

        v[16]={{  h, 0.f,  h },{ 0.f, 0.f }};
        v[17]={{  h, 0.f, -h },{ 1.f, 0.f }};
        v[18]={{  h, 1.f,  h },{ 0.f, 1.f }};
        v[19]={{  h, 1.f, -h },{ 1.f, 1.f }};

        v[20]={{ -h, 0.f, -h },{ 0.f, 0.f }};
        v[21]={{ -h, 0.f,  h },{ 1.f, 0.f }};
        v[22]={{ -h, 1.f, -h },{ 0.f, 1.f }};
        v[23]={{ -h, 1.f,  h },{ 1.f, 1.f }};

        uint32_t i = 0;
        for (uint32_t f = 0; f < 6; ++f)
        {
            const uint32_t b = f * 4;
            idx[i++]=b+2; idx[i++]=b+1; idx[i++]=b;
            idx[i++]=b+2; idx[i++]=b+3; idx[i++]=b+1;
        }

        vb->endVerticesData();
    }

    {
        auto*     vb   = m_renderer->getVertexBuffer(m_vVB);
        Instance* inst = reinterpret_cast<Instance *>(vb->beginInstancesData());
        uint32_t  cntr = 0;
        for (int xx = -20; xx < 20; ++xx)
            for (int zz = -20; zz < 20; ++zz)
            {
                inst[cntr].transform = glm::vec4(static_cast<float>(xx), 0.f, static_cast<float>(zz), 1.f);
                inst[cntr].color     = glm::vec4(1.f);
                ++cntr;
            }
        vb->endInstancesData();
    }

    m_vGround = m_renderer->createVertexBuffer(4, 6, 1,
        std::make_unique<S_VertexBufferDescriptorArray>(static_cast<uint32_t>(sizeof(Vertex)),   vertexDescs),
        std::make_unique<S_VertexBufferDescriptorArray>(static_cast<uint32_t>(sizeof(Instance)), instanceDescs));

    {
        auto*     ground = m_renderer->getVertexBuffer(m_vGround);
        auto      gbr    = ground->beginVerticesData();
        Vertex*   gv     = reinterpret_cast<Vertex *>(gbr.first);
        uint32_t* gidx   = reinterpret_cast<uint32_t *>(gbr.second);

        gv[0] = {{ -22.f, 0.f, -22.f }, { 0.f,  0.f }};
        gv[1] = {{  22.f, 0.f, -22.f }, { 22.f, 0.f }};
        gv[2] = {{ -22.f, 0.f,  22.f }, { 0.f,  22.f }};
        gv[3] = {{  22.f, 0.f,  22.f }, { 22.f, 22.f }};
        gidx[0]=0; gidx[1]=1; gidx[2]=2; gidx[3]=2; gidx[4]=1; gidx[5]=3;

        ground->endVerticesData();
    }

    {
        auto*     ground = m_renderer->getVertexBuffer(m_vGround);
        Instance* ginst  = reinterpret_cast<Instance *>(ground->beginInstancesData());
        ginst[0].transform = glm::vec4(0.f, -0.65f, 0.f, 0.f);
        ginst[0].color     = glm::vec4(1.0f, 1.0f, 1.0f, 1.f);
        ground->endInstancesData();
    }

    m_vTexture = m_renderer->createTexture("textures/sign.ktx");
    m_vSampler = m_renderer->createTextureSampler(S_TextureSamplerDescriptor());
    m_renderer->getTexture(m_vTexture)->setSampler(m_renderer->getSampler(m_vSampler));

    m_vCam = std::make_shared<S_CameraPerspective>();
    m_vCam->setPosition(glm::vec3(0.f, 28.f, 40.f));
    m_vCamController = std::make_shared<S_FirstPersonCameraController>();
    m_vCamController->setCamera(m_vCam);

    m_renderer->setRenderCallback([this]()
    {
        struct UPerObjectVS { glm::mat4 MVP; } uVS;
        struct UPerObjectFS { glm::vec4 Color; } uFS;
        struct Instance { glm::vec4 transform; glm::vec4 color; };

        const float elapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - m_startTime).count();

        auto* vb     = m_renderer->getVertexBuffer(m_vVB);
        auto* ground = m_renderer->getVertexBuffer(m_vGround);
        auto* shader = m_renderer->getShader(m_vShader);
        auto* tex    = m_renderer->getTexture(m_vTexture);

        Instance* inst = reinterpret_cast<Instance *>(vb->beginInstancesData());
        uint32_t  cntr = 0;
        for (int xx = -20; xx < 20; ++xx)
            for (int zz = -20; zz < 20; ++zz)
            {
                const float y = 0.6f * std::sinf(xx * 0.3f + zz * 0.2f + elapsed * 2.0f);
                inst[cntr].transform = glm::vec4(static_cast<float>(xx), y, static_cast<float>(zz), 1.f);
                inst[cntr].color     = glm::vec4(1.f);
                ++cntr;
            }
        vb->endInstancesData();

        m_vCam->setWidth(static_cast<float>(window()->width()));
        m_vCam->setHeight(static_cast<float>(window()->height()));
        m_vCamController->update();

        uVS.MVP   = m_vCam->viewProjection();
        uFS.Color = glm::vec4(1.f);

        shader->bind();
        shader->updateTextureValue("texSampler", S_ShaderStage::FragmentShader, *tex);
        shader->updateUniformValue("UPerObject", S_ShaderStage::VertexShader,   &uVS);
        shader->updateUniformValue("UPerObject", S_ShaderStage::FragmentShader, &uFS);
        shader->commit();
        ground->draw();
        vb->draw();

        {
            S_PerFrameData pfd;
            pfd.VP   = m_vCam->viewProjection();
            pfd.time = elapsed;
            m_renderer->updatePerFrame(pfd);
        }

        {
            ImGui::Begin("Solo Debug");
            ImGui::Text("%.1f fps (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
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
        {
            auto* foxMesh = m_renderer->getMesh(m_foxMesh);
            glm::vec4 r = m_ui->anchoredRect(0.5f, 1.0f, 0.0f, -95.0f, 430.0f, 150.0f);
            m_ui->panel(r.x, r.y, r.z, r.w);
            m_ui->text("SOLO ENGINE", r.x + r.z * 0.5f, r.y + 16.0f, 28.0f,
                       S_UI::rgba(235, 240, 255), true);

            if (foxMesh && !foxMesh->animations().empty())
            {
                int  clip = m_foxAnimator->clip();
                int  next = (clip + 1) % static_cast<int>(foxMesh->animations().size());
                if (m_ui->button("nextClip", "Play: " + foxMesh->animations()[next].name,
                                 r.x + r.z * 0.5f - 120.0f, r.y + 62.0f, 240.0f, 56.0f))
                    m_foxAnimator->setClip(static_cast<uint32_t>(next));
            }
        }

        m_foxAnimator->update(static_cast<float>(m_renderer->elapsedTimeUs()) / 1000000.0f);

        m_renderer->clearDraws();
        for (uint32_t n = 0; n < static_cast<uint32_t>(m_scene.nodes().size()); ++n)
        {
            const auto& node = m_scene.nodes()[n];
            if (n == m_foxNode)
                m_renderer->submitDraw(node.mesh, node.transform, node.materialID, m_foxAnimator->palette());
            else
                m_renderer->submitDraw(node.mesh, node.transform, node.materialID);
        }
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
