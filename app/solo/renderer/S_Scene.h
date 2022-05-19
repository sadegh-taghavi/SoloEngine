#pragma once
#include <memory>

namespace solo
{

class S_Camera;
class S_Scene
{
public:
    S_Scene();
    virtual ~S_Scene();
    std::shared_ptr<S_Camera> camera() const;
    void setCamera(const std::shared_ptr<S_Camera> &camera);

private:
    std::shared_ptr<S_Camera> m_camera;

};

}

