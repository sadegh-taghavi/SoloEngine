//#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtx/matrix_decompose.hpp>
#include "S_Mat4x4.h"
#include "S_Vec3.h"
#include "S_Quat.h"

inline S_Mat4x4::S_Mat4x4(	float i_00, float i_01, float i_02, float i_03,
                        float i_10, float i_11, float i_12, float i_13,
                        float i_20, float i_21, float i_22, float i_23,
                        float i_30, float i_31, float i_32, float i_33) :
    m_data( i_00, i_01, i_02, i_03,
    i_10, i_11, i_12, i_13,
    i_20, i_21, i_22, i_23,
    i_30, i_31, i_32, i_33 )
{
}

inline S_Mat4x4::S_Mat4x4( const float *i_array )
{
    memcpy( &m_data[0], i_array, sizeof(float) * 16 );
}

inline S_Mat4x4::S_Mat4x4( const S_Mat4x4 &i_mat44 )
{
    *this = i_mat44;
}

inline float S_Mat4x4::operator()( size_t i_row, size_t i_column ) const
{
    return m_data[i_row][i_column];
}

inline float& S_Mat4x4::operator()( size_t i_row, size_t i_column )
{
    return m_data[i_row][i_column];
}

inline S_Mat4x4& S_Mat4x4::operator=( const S_Mat4x4& i_mat44 )
{
    m_data = i_mat44.m_data;
    return *this;
}

inline S_Mat4x4 S_Mat4x4::operator*(const S_Mat4x4 &i_mat44)
{
    S_Mat4x4 m;
    m.m_data = m_data;
    m.m_data *= i_mat44.m_data;
    return m;
}

inline S_Mat4x4& S_Mat4x4::operator*=(const S_Mat4x4 &i_mat44)
{
    return multiply(&i_mat44);
}

inline S_Mat4x4& S_Mat4x4::multiply( const S_Mat4x4 *i_mat44 )
{
    m_data *= i_mat44;
    return *this;
}

inline void S_Mat4x4::multiplyOut( S_Mat4x4 *i_out, const S_Mat4x4 *i_mat44 )
{
    i_out->m_data *= i_mat44->m_data;
}

inline S_Mat4x4& S_Mat4x4::srp( const S_Vec3 &i_position, const S_Quat &i_rotation, const S_Vec3 &i_scale )
{
    glm::mat4 identity = glm::mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
    glm::mat4 p = glm::translate( identity, glm::vec3( i_position.x, i_position.y, i_position.z ) );
    glm::mat4 s = glm::scale( identity, glm::vec3( i_scale.x, i_scale.y, i_scale.z ) );
    glm::mat4 r = glm::mat4_cast( i_rotation.m_data );
    m_data = s * r * p;
    return *this;
}

inline S_Mat4x4& S_Mat4x4::spr( const S_Vec3 &i_position, const S_Quat &i_rotation, const S_Vec3 &i_scale )
{
    glm::mat4 identity = glm::mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
    glm::mat4 p = glm::translate( identity, glm::vec3( i_position.x, i_position.y, i_position.z ) );
    glm::mat4 s = glm::scale( identity, glm::vec3( i_scale.x, i_scale.y, i_scale.z ) );
    glm::mat4 r = glm::mat4_cast( i_rotation.m_data );
    m_data = s * p * r;
    return *this;
}

inline S_Mat4x4& S_Mat4x4::transform( const S_Vec3 &i_p )
{
    m_data = glm::translate( m_data, i_p.m_data );
    return *this;
}

inline void S_Mat4x4::transformOut( S_Mat4x4 *i_out, const S_Vec3 &i_p )
{
    i_out->m_data = glm::translate( m_data, i_p.m_data );
}


inline S_Mat4x4& S_Mat4x4::lookAtRH( const S_Vec3 *i_position, const S_Vec3 *i_target, const S_Vec3 *i_up )
{

    m_data = glm::lookAtRH( i_position->m_data, i_target->m_data, i_up->m_data );
    return *this;
}

inline S_Mat4x4& S_Mat4x4::inverse()
{
    m_data = glm::inverse( m_data );
    return *this;
}

inline S_Mat4x4& S_Mat4x4::inverseBy( const S_Mat4x4 *i_mat )
{
    m_data = glm::inverse( i_mat->m_data );
    return *this;
}

inline void S_Mat4x4::inverseOut( S_Mat4x4 *i_out )
{
    i_out->m_data = glm::inverse( m_data );
}

inline S_Mat4x4& S_Mat4x4::orthoRH( float i_w, float i_h, float i_near, float i_far )
{
    float halfW = i_w * 0.5f;
    float halfH = i_h * 0.5f;
    m_data = glm::orthoRH( -halfW, halfW, -halfH, halfH, i_near, i_far );
    return *this;
}

inline S_Mat4x4& S_Mat4x4::orthoCenterRH( float i_wLeft, float i_wRight, float i_hBottom, float i_hTop, float i_near, float i_far )
{
    m_data = glm::orthoRH( i_wLeft, i_wRight, i_hBottom, i_hTop, i_near, i_far );
    return *this;
}


inline S_Mat4x4& S_Mat4x4::perspectiveFovRH( float i_fov, float i_width, float i_height, float i_near, float i_far )
{
    m_data = glm::perspectiveFovRH( i_fov, i_width, i_height, i_near, i_far );
    return *this;
}

inline S_Mat4x4& S_Mat4x4::transpose()
{
    m_data = glm::transpose( m_data );
    return *this;
}

inline S_Mat4x4& S_Mat4x4::transposeBy( const S_Mat4x4 *i_mat )
{
    m_data = glm::transpose( i_mat->m_data );
    return *this;
}

inline void S_Mat4x4::transposeOut( S_Mat4x4 *i_out )
{
    i_out->m_data = glm::transpose( i_out->m_data );
}

inline S_Mat4x4& S_Mat4x4::identity()
{
    memset(this, 0, sizeof(S_Mat4x4));
    m_data[0][0] = 1.0f;
    m_data[1][1] = 1.0f;
    m_data[2][2] = 1.0f;
    m_data[3][3] = 1.0f;
    return *this;
}

inline S_Mat4x4& S_Mat4x4::rotationQuaternion( const S_Quat *i_q )
{
    m_data = glm::mat4_cast( i_q->m_data );
    return *this;
}

//inline void S_Mat4x4::decompose( S_Vec3 *i_outPosition, S_Quat *i_outRotation, S_Vec3 *i_outScale,  S_Vec3 *i_outSkew,  S_Vec3 *i_outPerspective )
//{
//    glm::decompose( m_data, i_outScale->m_data, i_outRotation->m_data, i_outPosition->m_data, i_outSkew->m_data, i_outPerspective->m_data );
//}

//inline S_Mat4x4& S_Mat4x4::transformation2D( const S_Vec2 *i_scaleCenter, float i_scalingRotation, const S_Vec2 *i_scale, const S_Vec2 *i_rotationCenter, float i_rotation, const S_Vec2 *i_position )
//{
//    DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this,
//        DirectX::XMMatrixTransformation2D(
//        DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)i_scaleCenter),
//        i_scalingRotation,
//        DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)i_scale),
//        DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)i_rotationCenter),
//        i_rotation,
//        DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)i_position)
//        )
//        );

//    return *this;
//}
