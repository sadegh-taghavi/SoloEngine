#include "S_CameraController.h"
#include "S_Camera.h"
#include "solo/application/S_Application.h"
#include "solo/math/S_Math.h"
#include <cmath>

using namespace solo;


S_CameraController::S_CameraController()
{

}

S_CameraController::S_CameraController(const std::shared_ptr<S_Camera> &camera): m_camera(camera)
{

}

S_CameraController::~S_CameraController()
{

}

void S_CameraController::update(float)
{

}

std::shared_ptr<S_Camera> S_CameraController::camera() const
{
    return m_camera;
}

void S_CameraController::setCamera(const std::shared_ptr<S_Camera> &camera)
{
    m_camera = camera;
}

S_FirstPersonCameraController::S_FirstPersonCameraController()
{

}

S_FirstPersonCameraController::S_FirstPersonCameraController(const std::shared_ptr<S_Camera> &camera) : S_CameraController (camera)
{
    syncFromCamera();
}

S_FirstPersonCameraController::~S_FirstPersonCameraController()
{

}

void S_FirstPersonCameraController::setCamera(const std::shared_ptr<S_Camera> &camera)
{
    S_CameraController::setCamera( camera );
    syncFromCamera();
}

void S_FirstPersonCameraController::syncFromCamera()
{
    if( m_camera == nullptr )
        return;
    m_position = m_camera->position();
    m_velocity = glm::vec3(0.0f);

    const glm::vec3 diff = m_camera->target() - m_camera->position();
    if( glm::dot(diff, diff) > 1e-8f )
    {
        const glm::vec3 d = glm::normalize(diff);
        m_pitch = std::asin(glm::clamp(d.y, -1.0f, 1.0f));
        m_yaw   = std::atan2(d.x, -d.z);
    }
}

void S_FirstPersonCameraController::update(float dtSeconds)
{
    if( m_camera == nullptr || dtSeconds <= 0.0f )
        return;
    auto* input = S_Application::executingApplication()->inputState();

    // ---- look: right mouse drag, explicit yaw/pitch (no roll, no flip) ----
    if( input->isKey(S_MouseButton::Right) )
    {
        constexpr float kRadPerPixel = 0.0035f;
        m_yaw   += input->mouseDeltaX() * kRadPerPixel;
        m_pitch -= input->mouseDeltaY() * kRadPerPixel;
        m_pitch  = glm::clamp(m_pitch, -1.55f, 1.55f);
    }

    const float cp = std::cos(m_pitch);
    const glm::vec3 forward(std::sin(m_yaw) * cp, std::sin(m_pitch), -std::cos(m_yaw) * cp);
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    // ---- speed: wheel scales the base, shift sprints ----
    const int wheel = input->mouseZ();
    if( wheel != 0 )
        m_baseSpeed = glm::clamp(m_baseSpeed * std::pow(1.15f, wheel / 120.0f), 0.5f, 200.0f);

    // ---- move: WASD planar, Q/E vertical, frame-rate independent ----
    glm::vec3 wish(0.0f);
    if( input->isKey(S_Key::W) ) wish += forward;
    if( input->isKey(S_Key::S) ) wish -= forward;
    if( input->isKey(S_Key::D) ) wish += right;
    if( input->isKey(S_Key::A) ) wish -= right;
    if( input->isKey(S_Key::E) ) wish += up;
    if( input->isKey(S_Key::Q) ) wish -= up;
    if( glm::dot(wish, wish) > 0.0f )
        wish = glm::normalize(wish);

    const float speed = m_baseSpeed * (input->isKey(S_Key::LeftShift) ? 4.0f : 1.0f);

    // exponential velocity smoothing (critically-damped feel, dt-correct)
    const float blend = 1.0f - std::exp(-12.0f * dtSeconds);
    m_velocity = glm::mix(m_velocity, wish * speed, blend);
    m_position += m_velocity * dtSeconds;

    m_camera->setPosition(m_position);
    m_camera->setTarget(m_position + forward);
    m_camera->update();
}
