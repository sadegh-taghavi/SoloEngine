#include "S_CameraController.h"
#include "S_Camera.h"
#include "solo/application/S_Application.h"
#include "solo/math/S_Quat.h"
#include "solo/math/S_Mat4x4.h"
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

//    m_rotAnimation.setDurationStart(200);
//    m_rotAnimation.setDurationRepeat(22);
//    m_rotAnimation.setDurationEnd(1);
//    m_rotAnimation.setEasingTypeStart(S_EasingType::LinearInterpolation);
//    m_rotAnimation.setEasingTypeRepeat(S_EasingType::LinearInterpolation);
//    m_rotAnimation.setEasingTypeEnd(S_EasingType::LinearInterpolation);
//    m_rotAnimation.setCurrent( (m_camera->target() - m_camera->position()).normalize() );
//    m_rotAnimation.setTo( m_rotAnimation.current() );

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
    bool rotChanged = false;
    if( app->inputState()->isKey( S_MouseButton::Left ) )
    {
        posX = static_cast<float>(app->inputState()->mouseX() ) / static_cast<float>( app->window()->width() ) * 2.0f - 1.0f;
        posY = static_cast<float>( app->inputState()->mouseY() ) / static_cast<float>( app->window()->height() ) * 2.0f - 1.0f;

        if( posY >= -0.8f && posY <= 0.8f && posX >= -0.8f && posX <= 0.8f )
        {
            rotX = app->inputState()->mouseDeltaX() / static_cast<float>( app->window()->width() ) * 1.0f;
            rotY = app->inputState()->mouseDeltaY() / static_cast<float>( app->window()->height() ) * -1.0f;
        }
    }

    S_Vec3 pos = m_posAnimation.to();
    S_Vec3 upVector = m_camera->up();
    S_Vec3 targetVector = (m_camera->target() - m_camera->position()).normalize();
    S_Vec3 sideVector;
    targetVector.crossOut( sideVector, upVector );

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

    if( app->inputState()->isKey( S_Key::A ) || posX < -0.8f )
    {
        pos += sideVector * step;
        posChanged = true;
    }
    else if( app->inputState()->isKey( S_Key::D ) || posX > 0.8f )
    {
        pos -= sideVector * step;
        posChanged = true;
    }

    if( rotX != 0.0f || rotY != 0.0f )
    {
        rotChanged = true;
        targetVector += sideVector * rotX;
        targetVector += upVector * rotY;
        targetVector.normalize();
    }

    if( posChanged )
        m_posAnimation.setTo( pos );

    m_posAnimation.update();   

    S_Vec3 calcPos = m_posAnimation.current();

//    if( rotChanged )
//        m_rotAnimation.setTo( targetVector );

//    m_rotAnimation.update();

    m_camera->setPosition( calcPos );

    m_camera->setTarget( targetVector + calcPos );

//    s_debug( "DDDDDDDDDDDD",  m_rotAnimation.current().x(),  m_rotAnimation.current().y(), m_rotAnimation.current().z() );

    m_camera->update();
}
