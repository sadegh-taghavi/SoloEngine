#include "S_Application.h"
#include "solo/debug/S_Debug.h"
#include "solo/file/S_File.h"
#include "solo/math/S_Math.h"
#include "solo/renderer/S_RendererAPI.h"
#include "solo/renderer/S_Shader.h"
#include "solo/renderer/S_Texture.h"
#include "solo/renderer/S_Camera.h"
#include "solo/renderer/S_CameraController.h"
#include <list>

using namespace solo;

solo::S_Application::S_Application(unsigned int width, unsigned int height) : S_BaseApplication ( width, height )
{

}

void solo::S_Application::onCreateEvent()
{
    s_debugLayer( "onCreateEvent" );
    m_startTime = std::chrono::steady_clock::now();
    m_resourceManager = std::make_unique<S_ResourceManager>();
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

    m_vShader = m_renderer->createShader("sr:/shaders/vs", "sr:/shaders/ps", "", "");

    S_PipelineDescriptor pd;
    pd.VertexBufferDescriptorArray   = S_VertexBufferDescriptorArray(static_cast<uint32_t>(sizeof(Vertex)),   vertexDescs);
    pd.InstanceBufferDescriptorArray = S_VertexBufferDescriptorArray(static_cast<uint32_t>(sizeof(Instance)), instanceDescs);
    pd.Shader = m_vShader;
    m_renderer->createGraphicsPipeline({ pd });

    m_vVB = m_renderer->createVertexBuffer(24, 36, 1600,
        std::make_unique<S_VertexBufferDescriptorArray>(static_cast<uint32_t>(sizeof(Vertex)),   vertexDescs),
        std::make_unique<S_VertexBufferDescriptorArray>(static_cast<uint32_t>(sizeof(Instance)), instanceDescs));

    {
        auto vbr = m_vVB->beginVerticesData();
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
            idx[i++]=b+2;   idx[i++]=b+1; idx[i++]=b;
            idx[i++]=b+2; idx[i++]=b+3; idx[i++]=b+1;
        }

        m_vVB->endVerticesData();
    }

    {
        Instance *inst = reinterpret_cast<Instance *>(m_vVB->beginInstancesData());
        uint32_t cntr = 0;
        for (int xx = -20; xx < 20; ++xx)
            for (int zz = -20; zz < 20; ++zz)
            {
                inst[cntr].transform = glm::vec4(static_cast<float>(xx), 0.f, static_cast<float>(zz), 1.f);
                inst[cntr].color     = glm::vec4(1.f);
                ++cntr;
            }
        m_vVB->endInstancesData();
    }

    m_vGround = m_renderer->createVertexBuffer(4, 6, 1,
        std::make_unique<S_VertexBufferDescriptorArray>(static_cast<uint32_t>(sizeof(Vertex)),   vertexDescs),
        std::make_unique<S_VertexBufferDescriptorArray>(static_cast<uint32_t>(sizeof(Instance)), instanceDescs));

    {
        auto gbr = m_vGround->beginVerticesData();
        Vertex   *gv   = reinterpret_cast<Vertex *>(gbr.first);
        uint32_t *gidx = reinterpret_cast<uint32_t *>(gbr.second);

        gv[0] = {{ -22.f, 0.f, -22.f }, { 0.f,  0.f }};
        gv[1] = {{  22.f, 0.f, -22.f }, { 22.f, 0.f }};
        gv[2] = {{ -22.f, 0.f,  22.f }, { 0.f,  22.f }};
        gv[3] = {{  22.f, 0.f,  22.f }, { 22.f, 22.f }};
        gidx[0]=0; gidx[1]=1; gidx[2]=2; gidx[3]=2; gidx[4]=1; gidx[5]=3;

        m_vGround->endVerticesData();
    }

    {
        Instance *ginst = reinterpret_cast<Instance *>(m_vGround->beginInstancesData());
        ginst[0].transform = glm::vec4(0.f, -0.65f, 0.f, 0.f);
        ginst[0].color     = glm::vec4(1.0f, 1.0f, 1.0f, 1.f);
        m_vGround->endInstancesData();
    }

    m_vTexture = m_renderer->createTexture("sr:/textures/sign.ktx");
    m_vTexture->setSampler(m_renderer->createTextureSampler(S_TextureSamplerDescriptor()));

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

        Instance *inst = reinterpret_cast<Instance *>(m_vVB->beginInstancesData());
        uint32_t cntr = 0;
        for (int xx = -20; xx < 20; ++xx)
            for (int zz = -20; zz < 20; ++zz)
            {
                const float y = 0.6f * std::sinf(xx * 0.3f + zz * 0.2f + elapsed * 2.0f);
                inst[cntr].transform = glm::vec4(static_cast<float>(xx), y, static_cast<float>(zz), 1.f);
                inst[cntr].color     = glm::vec4(1.f);
                ++cntr;
            }
        m_vVB->endInstancesData();

        m_vCam->setWidth(static_cast<float>(window()->width()));
        m_vCam->setHeight(static_cast<float>(window()->height()));
        m_vCamController->update();

        uVS.MVP   = m_vCam->viewProjection();
        uFS.Color = glm::vec4(1.f);

        m_vShader->bind();
        m_vShader->updateTextureValue("texSampler", S_ShaderStage::FragmentShader, *m_vTexture);
        m_vShader->updateUniformValue("UPerObject", S_ShaderStage::VertexShader,   &uVS);
        m_vShader->updateUniformValue("UPerObject", S_ShaderStage::FragmentShader, &uFS);
        m_vShader->commit();
        m_vGround->draw();
        m_vVB->draw();
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

S_ResourceManager *S_Application::resourceManager() const
{
    return m_resourceManager.get();
}

S_Application *S_Application::executingApplication()
{
    return static_cast<S_Application *>( S_BaseApplication::executingApplication() );
}

