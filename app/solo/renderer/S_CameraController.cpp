#include "S_CameraController.h"
#include "S_Camera.h"
#include "solo/application/S_Application.h"
#include "solo/math/S_Math.h"

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

void S_CameraController::update()
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

}

S_FirstPersonCameraController::~S_FirstPersonCameraController()
{

}

void S_FirstPersonCameraController::setCamera(const std::shared_ptr<S_Camera> &camera)
{
    S_CameraController::setCamera( camera );
    if( m_camera == nullptr )
        return;
    m_posAnimation.setCurrent( camera->position() );
    m_posAnimation.setTo( camera->position() );
}

void S_FirstPersonCameraController::update()
{
    if( m_camera == nullptr )
        return;
    auto app = S_Application::executingApplication();

    float rotX = 0.0f;
    float rotY = 0.0f;
    float posX = 0.0f;
    float posY = 0.0f;
    bool posChanged = false;
    const float winW = static_cast<float>( app->window()->width() );
    const float winH = static_cast<float>( app->window()->height() );
    if( winW > 0.0f && winH > 0.0f && app->inputState()->isKey( S_MouseButton::Left ) )
    {
        posX = static_cast<float>(app->inputState()->mouseX()) / winW * 2.0f - 1.0f;
        posY = static_cast<float>(app->inputState()->mouseY()) / winH * 2.0f - 1.0f;

        if( posY >= -0.8f && posY <= 0.8f && posX >= -0.8f && posX <= 0.8f )
        {
            rotX = app->inputState()->mouseDeltaX() / winW;
            rotY = app->inputState()->mouseDeltaY() / winH * -1.0f;
        }
    }

    glm::vec3 pos = m_posAnimation.to();
    glm::vec3 upVector = m_camera->up();
    glm::vec3 diff = m_camera->target() - m_camera->position();
    glm::vec3 targetVector = ( glm::dot(diff, diff) > 1e-8f )
                             ? glm::normalize(diff)
                             : glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 sideVector = glm::cross(targetVector, upVector);

    float step = 1.0f;

    if( app->inputState()->isKey( S_Key::W ) || posY < -0.8f )
    {
        pos += targetVector * step;
        posChanged = true;
    }
    else if( app->inputState()->isKey( S_Key::S ) || posY > 0.8f )
    {
        pos -= targetVector * step;
        posChanged = true;
    }

    if( app->inputState()->isKey( S_Key::D ) || posX < -0.8f )
    {
        pos += sideVector * step;
        posChanged = true;
    }
    else if( app->inputState()->isKey( S_Key::A ) || posX > 0.8f )
    {
        pos -= sideVector * step;
        posChanged = true;
    }

    if( rotX != 0.0f || rotY != 0.0f )
    {
        targetVector += sideVector * rotX;
        targetVector += upVector * rotY;
        float len2 = glm::dot( targetVector, targetVector );
        if( len2 > 1e-8f )
            targetVector = targetVector / std::sqrt( len2 );
    }

    if( posChanged )
        m_posAnimation.setTo( pos );

    m_posAnimation.update();

    glm::vec3 calcPos = m_posAnimation.current();

    m_camera->setPosition( calcPos );
    m_camera->setTarget( targetVector + calcPos );
    m_camera->update();
}
