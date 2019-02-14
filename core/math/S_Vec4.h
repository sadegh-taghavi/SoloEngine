#pragma once
#include <glm/glm.hpp>

class S_Mat4x4;

class S_Vec4
{
    friend class S_Vec2;
    friend class S_Vec3;
    friend class S_Quat;
    friend class S_Mat4x4;

    glm::vec4 m_data;

public:

    S_Vec4(){}
    S_Vec4(float i_x, float i_y, float i_z, float i_w);
    S_Vec4(const float *i_array);
    S_Vec4(const S_Vec4 &i_vec);

    float x();
    void setX(float x);
    float y();
    void setY(float y);
    float z();
    void setZ(float z);
    float w();
    void setW(float w);

    S_Vec4 operator+(const S_Vec4 &i_v);
    S_Vec4& operator+=(const S_Vec4 &i_v);
    S_Vec4 operator-();
    S_Vec4 operator-(const S_Vec4 &i_v);
    S_Vec4& operator-=(const S_Vec4 &i_v);
    S_Vec4 operator*(float i_scaler);
    S_Vec4& operator*=(float i_scaler);
    S_Vec4 operator/(float i_scaler);
    S_Vec4& operator/=(float i_scaler);
    bool operator==(const S_Vec4 &i_v);
    bool operator!=(const S_Vec4 &i_v);
    float length();
    S_Vec4& transform(const struct S_Mat4x4 *i_mat);
    void transformOut( S_Vec4 *i_out, const S_Mat4x4 *i_mat );
    S_Vec4& normalize();
    S_Vec4& normalizeBy(const S_Vec4* i_vec);
    void normalizeOut(S_Vec4* i_out);
    float dot(const S_Vec4 *i_vec);
    void lerp( const S_Vec4 *i_second, float i_amount );
    void lerpOut( S_Vec4 *i_out, const S_Vec4 *i_second, float i_amount );
    void To2DLeftUpPosition( const S_Vec4 *i_position, const S_Mat4x4 *i_viewProjection );
    void To2DLeftUpPositionOut( S_Vec4 *i_out, const S_Vec4 *i_position, const S_Mat4x4 *i_viewProjection );
};
#include "S_Vec4.inl"
