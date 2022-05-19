#include "S_Camera.h"

using namespace solo;

S_Camera::S_Camera(S_CameraType type): m_type(type),
    m_position(0, 2.0, 3.0), m_target( 0, 0, 0 ), m_up( 0, -1.0, 0 ),
    m_near( 1.0 ), m_far( 1000.0 )
{

}

S_Camera::~S_Camera()
{

}

S_Mat4x4 S_Camera::view() const
{
    return m_view;
}

S_Mat4x4 S_Camera::projection() const
{
    return m_projection;
}

S_Mat4x4 S_Camera::viewProjection() const
{
    return m_viewProjection;
}

void S_Camera::update()
{
    m_view.lookAtRH( m_position, m_target, m_up );
    m_viewProjection = m_projection * m_view;
}

float S_Camera::far() const
{
    return m_far;
}

void S_Camera::setFar(float far)
{
    m_far = far;
}

float S_Camera::near() const
{
    return m_near;
}

void S_Camera::setNear(float near)
{
    m_near = near;
}

void S_Camera::setUp(const S_Vec3 &up)
{
    m_up = up;
}

void S_Camera::setTarget(const S_Vec3 &target)
{
    m_target = target;
}

void S_Camera::setPosition(const S_Vec3 &position)
{
    m_position = position;
}

S_Vec3 S_Camera::up() const
{
    return m_up;
}

S_Vec3 S_Camera::target() const
{
    return m_target;
}

S_Vec3 S_Camera::position() const
{
    return m_position;
}


S_CameraOrthographic::S_CameraOrthographic(): S_Camera(S_CameraType::Orthographic), m_left( -0.5 ), m_right( 0.5 ), m_bottom( -0.5 ), m_top( -0.5 )
{

}

S_CameraOrthographic::~S_CameraOrthographic()
{

}

void S_CameraOrthographic::update()
{
    m_projection.orthoCenterRH( m_left, m_right, m_bottom, m_top, m_near, m_far );
    S_Camera::update();
}

float S_CameraOrthographic::bottom() const
{
    return m_bottom;
}

void S_CameraOrthographic::setBottom(float bottom)
{
    m_bottom = bottom;
}

float S_CameraOrthographic::top() const
{
    return m_top;
}

void S_CameraOrthographic::setTop(float top)
{
    m_top = top;
}

float S_CameraOrthographic::right() const
{
    return m_right;
}

void S_CameraOrthographic::setRight(float right)
{
    m_right = right;
}

float S_CameraOrthographic::left() const
{
    return m_left;
}

void S_CameraOrthographic::setLeft(float left)
{
    m_left = left;
}

void S_CameraOrthographic::setOrtho(float left, float right, float bottom, float top)
{
    m_left = left;
    m_right = right;
    m_bottom = bottom;
    m_top = top;
}


S_CameraPerspective::S_CameraPerspective(): S_Camera ( S_CameraType::Perspective ), m_fov( 1.05f ), m_width( 128.0 ), m_height( 128.0 )
{

}

S_CameraPerspective::~S_CameraPerspective()
{

}

void S_CameraPerspective::setPerspective(float fov, float width, float height)
{
    m_fov = fov;
    m_width = width;
    m_height = height;
}

float S_CameraPerspective::fov() const
{
    return m_fov;
}

void S_CameraPerspective::setFov(float fov)
{
    m_fov = fov;
}

float S_CameraPerspective::width() const
{
    return m_width;
}

void S_CameraPerspective::setWidth(float width)
{
    m_width = width;
}

float S_CameraPerspective::height() const
{
    return m_height;
}

void S_CameraPerspective::setHeight(float height)
{
    m_height = height;
}

void S_CameraPerspective::update()
{
    m_projection.perspectiveFovRH( m_fov, m_width, m_height, m_near, m_far );
    S_Camera::update();
}
