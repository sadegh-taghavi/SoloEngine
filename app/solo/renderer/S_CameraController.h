#pragma once
#include <memory>
#include "solo/math/S_Math.h"

namespace solo
{

class S_Camera;

class S_CameraController
{
public:
    S_CameraController();
    S_CameraController(const std::shared_ptr<S_Camera> &camera);
    virtual ~S_CameraController();

    virtual void update(float dtSeconds);
    std::shared_ptr<S_Camera> camera() const;
    virtual void setCamera(const std::shared_ptr<S_Camera> &camera);

protected:
    std::shared_ptr<S_Camera> m_camera;
};

// Editor-style free-fly camera, frame-rate independent:
//   right mouse drag — look (yaw/pitch, pitch clamped, no roll)
//   W/A/S/D         — move on the view plane,  Q/E — down/up
//   Shift           — speed boost,  mouse wheel — scales base speed
// Movement velocity is exponentially smoothed for feel.
class S_FirstPersonCameraController : public S_CameraController
{
public:
    S_FirstPersonCameraController();
    S_FirstPersonCameraController(const std::shared_ptr<S_Camera> &camera);
    virtual ~S_FirstPersonCameraController();
    virtual void setCamera(const std::shared_ptr<S_Camera> &camera) override;
    virtual void update(float dtSeconds) override;

    void  setBaseSpeed(float unitsPerSecond) { m_baseSpeed = unitsPerSecond; }
    float baseSpeed() const                  { return m_baseSpeed; }

private:
    void syncFromCamera();

    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_velocity = glm::vec3(0.0f);
    float     m_yaw      = 0.0f; // radians around +Y; 0 looks down -Z
    float     m_pitch    = 0.0f; // radians, clamped to +-~89 deg
    float     m_baseSpeed = 12.0f;
};

}
