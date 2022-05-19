#include "S_Scene.h"
#include "S_Camera.h"
using namespace solo;

S_Scene::S_Scene()
{

}

S_Scene::~S_Scene()
{

}

std::shared_ptr<S_Camera> S_Scene::camera() const
{
    return m_camera;
}

void S_Scene::setCamera(const std::shared_ptr<S_Camera> &camera)
{
    m_camera = camera;
}
