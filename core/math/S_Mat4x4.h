#pragma once
#include <glm/glm.hpp>

class S_Vec2;
class S_Vec3;
class S_Vec4;
class S_Quat;

class S_Mat4x4
{
    friend class S_Vec2;
    friend class S_Vec3;
    friend class S_Vec4;
    friend class S_Quat;

    glm::mat4 m_data;

public:

    S_Mat4x4(){}
    S_Mat4x4(	float i_00, float i_01, float i_02, float i_03,
                float i_10, float i_11, float i_12, float i_13,
                float i_20, float i_21, float i_22, float i_23,
                float i_30, float i_31, float i_32, float i_33);
    S_Mat4x4(const float *i_array);
    S_Mat4x4(const S_Mat4x4 &i_mat44);

    float		operator()(size_t i_row, size_t i_column) const;
    float&		operator()(size_t i_row, size_t i_column);
    S_Mat4x4&	operator= (const S_Mat4x4& i_mat44);
    S_Mat4x4	operator*(const S_Mat4x4 &i_mat44);
    S_Mat4x4&	operator*=(const S_Mat4x4 &i_mat44);
    S_Mat4x4&	multiply(const S_Mat4x4 *i_mat44);
    void		multiplyOut(S_Mat4x4 *i_out, const S_Mat4x4 *i_mat44);
    S_Mat4x4&	srp(const S_Vec3 &i_position, const S_Quat &i_rotation, const S_Vec3 &i_scale);
    S_Mat4x4&	spr( const S_Vec3 &i_position, const S_Quat &i_rotation, const S_Vec3 &i_scale );
    S_Mat4x4&	translate( const S_Vec3 &i_p );
    void		translateOut( S_Mat4x4 *i_out, const S_Vec3 &i_p );
    S_Mat4x4&	lookAtRH(const S_Vec3 *i_position, const S_Vec3 *i_target, const S_Vec3 *i_up);
    S_Mat4x4&	inverse();
    S_Mat4x4&	inverseBy(const S_Mat4x4 *i_mat);
    void		inverseOut(S_Mat4x4 *i_out);
    S_Mat4x4&	orthoRH(float i_w, float i_h, float i_near, float i_far);
    S_Mat4x4&	orthoCenterRH( float i_wLeft, float i_wRight, float i_hBottom, float i_hTop, float i_near, float i_far );
    S_Mat4x4&	perspectiveFovRH( float i_fov, float i_width, float i_height, float i_near, float i_far );
    S_Mat4x4&	transpose();
    S_Mat4x4&	transposeBy(const S_Mat4x4 *i_mat);
    void		transposeOut(S_Mat4x4 *i_out);
    void		decompose(S_Vec3 *i_outPosition, S_Quat *i_outRotation, S_Vec3 *i_outScale , S_Vec3 *i_outSkew, S_Vec4 *i_outPerspective);
    S_Mat4x4&	identity();
    S_Mat4x4&	rotationQuaternion(const S_Quat *i_q);
//    S_Mat4x4&	transformation2D(const S_Vec2 *i_scaleCenter, float i_scalingRotation, const S_Vec2 *i_scale,
//        const S_Vec2 *i_rotationCenter, float i_rotation, const S_Vec2 *i_position);
};

#include "S_Mat4x4.inl"
