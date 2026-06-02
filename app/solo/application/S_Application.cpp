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
#include <random>

using namespace solo;

solo::S_Application::S_Application(unsigned int width, unsigned int height) : S_BaseApplication ( width, height )
{

}

void solo::S_Application::onCreateEvent()
{
    s_debugLayer( "onCreateEvent" );
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

    m_vVB = m_renderer->createVertexBuffer(4, 6, 1600,
        std::make_unique<S_VertexBufferDescriptorArray>(static_cast<uint32_t>(sizeof(Vertex)),   vertexDescs),
        std::make_unique<S_VertexBufferDescriptorArray>(static_cast<uint32_t>(sizeof(Instance)), instanceDescs));

    auto vbr = m_vVB->beginVerticesData();
    Vertex *v = reinterpret_cast<Vertex *>(vbr.first);
    uint32_t *idx = reinterpret_cast<uint32_t *>(vbr.second);
    v[0] = {{ -0.5f, 0.0f, -0.5f }, { 0.0f, 0.0f }};
    v[1] = {{  0.5f, 0.0f, -0.5f }, { 1.0f, 0.0f }};
    v[2] = {{ -0.5f, 0.0f,  0.5f }, { 0.0f, 1.0f }};
    v[3] = {{  0.5f, 0.0f,  0.5f }, { 1.0f, 1.0f }};
    idx[0]=0; idx[1]=1; idx[2]=2; idx[3]=2; idx[4]=1; idx[5]=3;
    m_vVB->endVerticesData();

    Instance *inst = reinterpret_cast<Instance *>(m_vVB->beginInstancesData());
    std::mt19937 mt(std::random_device{}());
    std::uniform_real_distribution<float> dist(-2.0f, 2.0f);
    uint32_t cntr = 0;
    for (int xx = -20; xx < 20; ++xx)
        for (int zz = -20; zz < 20; ++zz)
        {
            inst[cntr].transform = glm::vec4(xx, dist(mt), zz, 0.0f);
            inst[cntr].color     = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            ++cntr;
        }
    m_vVB->endInstancesData();

    m_vTexture = m_renderer->api()->createTexture("sr:/textures/sign.ktx");
    m_vTexture->setSampler(m_renderer->api()->createTextureSampler(S_TextureSamplerDescriptor()));

    m_vCam = std::make_shared<S_CameraPerspective>();
    m_vCam->setPosition(glm::vec3(0.0f, 2.0f, 10.0f));
    m_vCamController = std::make_shared<S_FirstPersonCameraController>();
    m_vCamController->setCamera(m_vCam);

    m_renderer->setRenderCallback([this]()
    {
        struct UPerObjectVS { glm::mat4 MVP; } uVS;
        struct UPerObjectFS { glm::vec4 Color; } uFS;

        m_vCam->setWidth(static_cast<float>(window()->width()));
        m_vCam->setHeight(static_cast<float>(window()->height()));
        m_vCamController->update();

        uVS.MVP = glm::mat4(1.0f) * m_vCam->viewProjection();
        uFS.Color = glm::vec4(1.0f);

        m_vShader->bind();
        m_vShader->updateTextureValue("texSampler", S_ShaderStage::FragmentShader, *m_vTexture);
        m_vShader->updateUniformValue("UPerObject", S_ShaderStage::VertexShader,   &uVS);
        m_vShader->updateUniformValue("UPerObject", S_ShaderStage::FragmentShader, &uFS);
        m_vShader->commit();
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

