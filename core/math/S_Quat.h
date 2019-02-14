#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class S_Vec3;

class S_Quat
{
    friend class S_Vec2;
    friend class S_Vec3;
    friend class S_Vec4;
    friend class S_Mat4x4;

    glm::quat m_data;

public:

    S_Quat(){}
    S_Quat(float i_x, float i_y, float i_z, float i_w);
    S_Quat(const float *i_array);
    S_Quat(const S_Quat &i_q);

    float x();
    void setX(float x);
    float y();
    void setY(float y);
    float z();
    void setZ(float z);
    float w();
    void setW(float w);

    bool operator==(const S_Quat &i_q);
    S_Quat& operator*=(const S_Quat &i_q);
    S_Quat operator*(const S_Quat &i_q);
    bool operator!=(const S_Quat &i_q);
    S_Quat& multiply(const S_Quat *i_mat44);
    void	 multiplyOut(S_Quat *i_out, const S_Quat *i_q);
    S_Quat& identity();
    S_Quat& angleAxis(float i_angle, const S_Vec3 *i_axis);
    void angleAxisOut(S_Quat *i_out, float i_angle, const S_Vec3 *i_axis);
    S_Quat& fromEularAnglesPYR(const S_Vec3 *i_pyr);
    void fromEularAnglesPYROut(S_Quat *i_out, const S_Vec3 *i_pyr);
    S_Quat& normalize();
    void normalizeOut(S_Quat *i_out);
    void toPYR(S_Vec3 *i_out);
    void lerp( const S_Quat *i_second, float i_amount );
    void lerpOut( S_Quat *i_out, const S_Quat *i_second, float i_amount );
};

#include "S_Quat.inl"
