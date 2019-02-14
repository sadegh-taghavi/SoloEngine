#include <glm/gtc/quaternion.hpp>
#include "S_Quat.h"
#include "S_Vec3.h"

inline S_Quat::S_Quat(float i_x, float i_y, float i_z, float i_w) : m_data( i_x, i_y, i_z, i_w )
{
}

inline S_Quat::S_Quat(const float *i_array) : m_data( i_array[0], i_array[1], i_array[2], i_array[3] )
{
}

inline S_Quat::S_Quat(const S_Quat &i_q)
{
	*this = i_q;
}


inline float S_Quat::x()
{
    return m_data.x;
}

inline void S_Quat::setX(float x)
{
    m_data[0] = x;
}

inline float S_Quat::y()
{
    return m_data.y;
}

inline void S_Quat::setY(float y)
{
    m_data.y = y;
}

inline float S_Quat::z()
{
    return m_data.z;
}

inline void S_Quat::setZ(float z)
{
    m_data.z = z;
}

inline float S_Quat::w()
{
    return m_data.w;
}

inline void S_Quat::setW(float w)
{
    m_data.w = w;
}

inline bool S_Quat::operator==(const S_Quat &i_q)
{
    if( m_data == i_q.m_data )
        return false;
    return true;
}

inline bool S_Quat::operator!=(const S_Quat &i_q)
{
	return !(*this == i_q);
}


inline S_Quat& S_Quat::operator*=( const S_Quat &i_q )
{
    m_data *= i_q.m_data;
    return *this;
}

inline S_Quat S_Quat::operator*( const S_Quat &i_q )
{
    S_Quat q = *this;
	q *= i_q;
	return q;
}

inline S_Quat& S_Quat::identity()
{
    m_data = glm::quat( 0.0f, 0.0f, 0.0f, 1.0f );
	return *this;
}

inline S_Quat& S_Quat::multiply( const S_Quat *i_q )
{
    m_data *= i_q->m_data;
	return *this;
}

inline void S_Quat::multiplyOut(S_Quat *i_out, const S_Quat *i_q)
{
    i_out->m_data = m_data * i_q->m_data;
}

inline S_Quat& S_Quat::fromAngleAxis( float i_angle, const S_Vec3 *i_axis)
{
    m_data = glm::angleAxis( i_angle, i_axis->m_data );
	return *this;
}

inline void S_Quat::fromAngleAxisOut(S_Quat *i_out, float i_angle, const S_Vec3 *i_axis)
{
    i_out->m_data = glm::angleAxis( i_angle, i_axis->m_data );
}

inline S_Quat& S_Quat::fromEularAnglesPYR( const S_Vec3 *i_pyr )
{
    m_data = glm::quat( i_pyr->m_data );
	return *this;
}

inline void S_Quat::fromEularAnglesPYROut( S_Quat *i_out, const S_Vec3 *i_pyr )
{
    i_out->m_data = glm::quat( i_pyr->m_data );
}

inline S_Quat& S_Quat::normalize()
{
    m_data = glm::normalize( m_data );
	return *this;
}

inline void S_Quat::normalizeOut(S_Quat *i_out)
{
    i_out->m_data = glm::normalize( m_data );
}

inline void S_Quat::toPYR( S_Vec3 *i_out )
{
    i_out->m_data = glm::eulerAngles( m_data );
}

inline void S_Quat::lerp( const S_Quat *i_second, float i_amount )
{
    glm::lerp( m_data, i_second->m_data, i_amount );
}

inline void S_Quat::lerpOut( S_Quat *i_out, const S_Quat *i_second, float i_amount )
{
    glm::lerp( i_out->m_data, i_second->m_data, i_amount );
}
