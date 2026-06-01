#pragma once

#include <solo/math/S_Math.h>

namespace solo
{

enum class S_CameraType
{
    Orthographic,
    Perspective,
};

class S_Camera
{

public:
    S_Camera(S_CameraType type);
    virtual ~S_Camera();

    glm::mat4 view() const;
    glm::mat4 projection() const;
    glm::mat4 viewProjection() const;
    glm::vec3 position() const;
    glm::vec3 target() const;
    glm::vec3 up() const;
    void setPosition(const glm::vec3 &position);
    void setTarget(const glm::vec3 &target);
    void setUp(const glm::vec3 &up);
    float near() const;
    void setNear(float near);
    float far() const;
    void setFar(float far);
    virtual void update();
protected:

    S_CameraType m_type;
    glm::vec3 m_position;
    glm::vec3 m_target;
    glm::vec3 m_up;
    glm::mat4 m_view;
    glm::mat4 m_projection;
    glm::mat4 m_viewProjection;
    float m_near;
    float m_far;
};

class S_CameraOrthographic: public S_Camera
{
public:
    S_CameraOrthographic();
    virtual ~S_CameraOrthographic();
    void setOrtho( float left, float right, float bottom, float top );
    float left() const;
    void setLeft(float left);
    float right() const;
    void setRight(float right);
    float top() const;
    void setTop(float top);
    float bottom() const;
    void setBottom(float bottom);
    virtual void update();
protected:
    float m_left;
    float m_right;
    float m_bottom;
    float m_top;

};

class S_CameraPerspective: public S_Camera
{
public:
    S_CameraPerspective();
    virtual ~S_CameraPerspective();
    void setPerspective( float fov, float width, float height );
    float fov() const;
    void setFov(float fov);
    float width() const;
    void setWidth(float width);
    float height() const;
    void setHeight(float height);
    virtual void update();
protected:
    float m_fov;
    float m_width;
    float m_height;
};

}
