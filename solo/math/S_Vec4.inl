#include "S_Vec4.h"
#include "S_Vec3.h"
#include "S_Mat4x4.h"

using namespace solo;

inline S_Vec4::S_Vec4( float i_x, float i_y, float i_z, float i_w ) : m_data( i_x, i_y, i_z, i_w )
{

}

inline S_Vec4::S_Vec4( const float *i_array ) : m_data( i_array[0], i_array[1], i_array[2], i_array[3] )
{

}

inline float S_Vec4::x()
{
    return m_data.x;
}

inline void S_Vec4::setX(float x)
{
    m_data[0] = x;
}

inline float S_Vec4::y()
{
    return m_data.y;
}

inline void S_Vec4::setY(float y)
{
    m_data.y = y;
}

inline float S_Vec4::z()
{
    return m_data.z;
}

inline void S_Vec4::setZ(float z)
{
    m_data.z = z;
}

inline float S_Vec4::w()
{
    return m_data.w;
}

inline void S_Vec4::setW(float w)
{
    m_data.w = w;
}

inline S_Vec4 S_Vec4::operator+( const S_Vec4 &i_v )
{
    S_Vec4 ret;
    ret.m_data = m_data + i_v.m_data;
    return ret;
}

inline S_Vec4& S_Vec4::operator+=( const S_Vec4 &i_v )
{
    m_data += i_v.m_data;
    return *this;
}

inline S_Vec4 S_Vec4::operator-( )
{
    S_Vec4 ret;
    ret.m_data = -m_data;
    return ret;
}

inline S_Vec4 S_Vec4::operator-( const S_Vec4 &i_v )
{
    S_Vec4 ret;
    ret.m_data = m_data - i_v.m_data;
    return ret;
}

inline S_Vec4& S_Vec4::operator-=( const S_Vec4 &i_v )
{
    m_data -= i_v.m_data;
    return *this;
}

inline S_Vec4 S_Vec4::operator*( float i_scaler )
{
    S_Vec4 ret;
    ret.m_data = m_data * i_scaler;
    return ret;
}

inline S_Vec4& S_Vec4::operator*=( float i_scaler )
{
    m_data *= i_scaler;
    return *this;
}

inline S_Vec4 S_Vec4::operator/( float i_scaler )
{
    S_Vec4 ret;
    ret.m_data = m_data / i_scaler;
    return ret;
}

inline S_Vec4& S_Vec4::operator/=( float i_scaler )
{
    m_data /= i_scaler;
    return *this;
}

inline bool S_Vec4::operator==( const S_Vec4 &i_v )
{
    if( m_data == i_v.m_data )
        return false;
    return true;
}

inline bool S_Vec4::operator!=( const S_Vec4 &i_v )
{
    return !( *this == i_v );
}

inline float S_Vec4::length()
{
    return glm::length( m_data );
}

inline S_Vec4& S_Vec4::transform(const S_Mat4x4 *i_mat)
{
    m_data = i_mat->m_data * m_data;
    return *this;
}

inline void S_Vec4::transformOut(S_Vec4 *i_out, const S_Mat4x4 *i_mat)
{
    i_out->m_data = i_mat->m_data * m_data;
}

inline S_Vec4& S_Vec4::normalize()
{
    m_data = glm::normalize( m_data );
    return *this;
}

inline S_Vec4& S_Vec4::normalizeBy( const S_Vec4* i_vec )
{
    m_data = glm::normalize( i_vec->m_data );
    return *this;
}

inline void S_Vec4::normalizeOut( S_Vec4* i_out )
{
    i_out->m_data = glm::normalize( m_data );
}

inline float S_Vec4::dot( const S_Vec4 *i_vec )
{
    return glm::dot( m_data, i_vec->m_data );
}

inline void S_Vec4::lerp( const S_Vec4 *i_second, float i_amount )
{
    m_data = glm::mix( m_data, i_second->m_data, i_amount );
}

inline void S_Vec4::lerpOut( S_Vec4 *i_out, const S_Vec4 *i_second, float i_amount )
{
    i_out->m_data = glm::mix( m_data, i_second->m_data, i_amount );
}

inline void S_Vec4::To2DLeftUpPosition( const S_Vec4 *i_position, const S_Mat4x4 *i_viewProjection )
{
    S_Vec4 pos( i_position->m_data.x, i_position->m_data.y, i_position->m_data.z, 1.0f );

    m_data = i_viewProjection->m_data * pos.m_data;
	    
    m_data.x /= m_data.w;
    m_data.y /= m_data.w;
    m_data.z /= m_data.w;
    m_data.x = m_data.x * 0.5f + 0.5f;
    m_data.y = ( -m_data.y * 0.5f + 0.5f );
}

inline void S_Vec4::To2DLeftUpPositionOut( S_Vec4 *i_out, const S_Vec4 *i_position, const S_Mat4x4 *i_viewProjection )
{
    S_Vec4 pos( i_position->m_data.x, i_position->m_data.y, i_position->m_data.z, 1.0f );

    i_out->m_data = i_viewProjection->m_data * pos.m_data;

    i_out->m_data.x /= i_out->m_data.w;
    i_out->m_data.y /= i_out->m_data.w;
    i_out->m_data.z /= i_out->m_data.w;
    i_out->m_data.x = i_out->m_data.x * 0.5f + 0.5f;
    i_out->m_data.y = ( -i_out->m_data.y * 0.5f + 0.5f );
}
