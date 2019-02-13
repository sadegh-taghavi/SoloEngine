#pragma once
#include <glm/glm.hpp>

class S_Mat4x4;

class S_Vec2
{
    friend class S_Vec3;
    friend class S_Vec4;
    friend class S_Quat;
    friend class S_Mat4x4;

    glm::vec2 m_data;

public:

    S_Vec2(){}
    S_Vec2(float i_x, float i_y);
    S_Vec2(const float *i_array);
    S_Vec2(const S_Vec2 &i_vec);

    float x();
    void setX(float x);
    float y();
    void setY(float y);

    S_Vec2 operator+(const S_Vec2 &i_v);
    S_Vec2& operator+=(const S_Vec2 &i_v);
    S_Vec2 operator-();
    S_Vec2 operator-(const S_Vec2 &i_v);
    S_Vec2& operator-=(const S_Vec2 &i_v);
    S_Vec2 operator*(float i_scaler);
    S_Vec2& operator*=(float i_scaler);
    S_Vec2 operator/(float i_scaler);
    S_Vec2& operator/=(float i_scaler);
    bool operator==(const S_Vec2 &i_v);
    bool operator!=(const S_Vec2 &i_v);
	float length();
    S_Vec2& normalize();
    void normalizeOut(S_Vec2* i_out);
    float dot(const S_Vec2 *i_vec);
    void lerp( const S_Vec2 *i_second, float i_amount );
    void lerpOut( S_Vec2 *i_out, const S_Vec2 *i_second, float i_amount );

};

#include "S_Vec2.inl"
