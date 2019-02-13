#pragma once
#include <glm/glm.hpp>

class S_Mat4x4;

class S_Vec3
{
    friend class S_Vec2;
    friend class S_Vec4;
    friend class S_Quat;
    friend class S_Mat4x4;

    glm::vec3 m_data;

public:

    S_Vec3(){}
    S_Vec3(float i_x, float i_y, float i_z);
    S_Vec3(const float *i_array);
    S_Vec3(const S_Vec3 &i_vec);

    float x();
    void setX(float x);
    float y();
    void setY(float y);
    float z();
    void setZ(float z);

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
    S_Vec3& normalizeBy(const S_Vec3* i_vec);
    void normalizeOut(S_Vec3* i_out);
    float dot(const S_Vec3 *i_vec);
    S_Vec3& cross(const S_Vec3 *i_vec);
    void crossOut(S_Vec3 *i_out, const S_Vec3 *i_vec);
    void lerp( const S_Vec3 *i_second, float i_amount );
    void lerpOut( S_Vec3 *i_out, const S_Vec3 *i_second, float i_amount );
};

//bool S_Vec3RayIntersectToPlane(const S_Vec3 *i_org, const S_Vec3 *i_dir, const S_Vec3 *i_verts, float &i_distance);
//bool S_Vec3RayIntersectToBox( const S_Vec3 *i_org, const S_Vec3 *i_dir,
//    const S_Vec3 *i_min, const S_Vec3 *i_max, const S_Mat4x4 *i_transform, float &i_distance );

#include "S_Vec3.inl"
