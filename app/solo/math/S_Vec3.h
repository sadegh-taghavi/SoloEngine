#pragma once
#include <glm/glm.hpp>

namespace solo
{

class S_Mat4x4;
class S_Vec4;

class S_Vec3
{
    friend class S_Vec2;
    friend class S_Vec4;
    friend class S_Quat;
    friend class S_Mat4x4;
    friend S_Vec3 min(S_Vec3 v1, S_Vec3 v2);
    friend S_Vec3 max(S_Vec3 v1, S_Vec3 v2);

    glm::vec3 m_data;

public:

    S_Vec3(){}
    S_Vec3(float i_x, float i_y, float i_z);
    S_Vec3(const float *i_array);
    S_Vec3(const double *i_array);
    S_Vec3(const S_Vec3 &i_vec);
    S_Vec3(const S_Vec4 &i_vec);

    float x();
    void setX(float x);
    float y();
    void setY(float y);
    float z();
    void setZ(float z);

    S_Vec3 operator=(const S_Vec4 &i_v);
    S_Vec3 operator=(const S_Vec3 &i_v);
    S_Vec3 operator+(const S_Vec3 &i_v);
    S_Vec3& operator+=(const S_Vec3 &i_v);
    S_Vec3 operator-();
    S_Vec3 operator-(const S_Vec3 &i_v);
    S_Vec3& operator-=(const S_Vec3 &i_v);
    S_Vec3 operator*(float i_scaler);
    S_Vec3& operator*=(float i_scaler);
    S_Vec3 operator/(float i_scaler);
    S_Vec3& operator/=(float i_scaler);
    bool operator==(const S_Vec3 &i_v);
    bool operator!=(const S_Vec3 &i_v);
	float length();
    S_Vec3& normalize();
    S_Vec3& normalizeBy(const S_Vec3& i_vec);
    void normalizeOut(S_Vec3& i_out);
    float dot(const S_Vec3 &i_vec);
    S_Vec3& cross(const S_Vec3 &i_vec);
    void crossOut(S_Vec3 &i_out, const S_Vec3 &i_vec);
    void lerp( const S_Vec3 &i_second, float i_amount );
    void lerpOut( S_Vec3 &i_out, const S_Vec3 &i_second, float i_amount );
};

}

#include "S_Vec3.inl"
