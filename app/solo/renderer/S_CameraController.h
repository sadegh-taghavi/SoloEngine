#pragma once
#include <memory>
#include "solo/math/S_Vec3.h"
#include "solo/utility/S_Behavior.h"

namespace solo
{

class S_Camera;

class S_CameraController
{
public:
    S_CameraController();
    S_CameraController(const std::shared_ptr<S_Camera> &camera);
    virtual ~S_CameraController();

    virtual void update();
    std::shared_ptr<S_Camera> camera() const;
    virtual void setCamera(const std::shared_ptr<S_Camera> &camera);

protected:
    std::shared_ptr<S_Camera> m_camera;
};

class S_FirstPersonCameraController : public S_CameraController
{
public:
    S_FirstPersonCameraController();
    S_FirstPersonCameraController(const std::shared_ptr<S_Camera> &camera);
    virtual ~S_FirstPersonCameraController();
    virtual void setCamera(const std::shared_ptr<S_Camera> &camera);
    virtual void update();

protected:
    S_Behavior<S_Vec3> m_posAnimation;
    S_Behavior<S_Vec3> m_rotAnimation;


};


}

