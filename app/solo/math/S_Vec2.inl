#pragma once
#include "S_Vec2.h"
#include "S_Mat4x4.h"
#include <glm/vec2.hpp>

using namespace solo;

inline S_Vec2::S_Vec2(float i_x, float i_y) : m_data( i_x, i_y )
{

}

inline S_Vec2::S_Vec2(const float *i_array)
{
    m_data = glm::make_vec2( i_array );
}

inline S_Vec2::S_Vec2(const double *i_array)
{
    m_data = glm::make_vec2( i_array );
}

inline S_Vec2::S_Vec2(const S_Vec2 &i_vec) : m_data( i_vec.m_data )
{

}

inline S_Vec2 S_Vec2::operator=(const S_Vec2 &i_v)
{
    this->m_data = i_v.m_data;
    return *this;
}

inline S_Vec2 S_Vec2::operator+(const S_Vec2 &i_v)
{
    S_Vec2 ret;
    ret.m_data = m_data + i_v.m_data;
    return ret;
}

inline float S_Vec2::x()
{
    return m_data.x;
}

inline void S_Vec2::setX(float x)
{
    m_data[0] = x;
}

inline float S_Vec2::y()
{
    return m_data.y;
}

inline void S_Vec2::setY(float y)
{
    m_data.y = y;
}

inline S_Vec2& S_Vec2::operator+=(const S_Vec2 &i_v)
{
    m_data += i_v.m_data;
    return *this;
}

inline S_Vec2 S_Vec2::operator-()
{
    S_Vec2 ret;
    ret.m_data = -m_data;
    return ret;
}

inline S_Vec2 S_Vec2::operator-(const S_Vec2 &i_v)
{
    S_Vec2 ret;
    ret.m_data = m_data - i_v.m_data;
    return ret;
}

inline S_Vec2& S_Vec2::operator-=(const S_Vec2 &i_v)
{
    m_data -= i_v.m_data;
    return *this;
}

inline S_Vec2 S_Vec2::operator*(float i_scaler)
{
    S_Vec2 ret;
    ret.m_data = m_data * i_scaler;
    return ret;
}

inline S_Vec2& S_Vec2::operator*=(float i_scaler)
{
    m_data *= i_scaler;
    return *this;
}

inline S_Vec2 S_Vec2::operator/(float i_scaler)
{
    S_Vec2 ret;
    ret.m_data = m_data / i_scaler;
    return ret;
}

inline S_Vec2& S_Vec2::operator/=(float i_scaler)
{
    m_data /= i_scaler;
    return *this;
}

inline bool S_Vec2::operator==(const S_Vec2 &i_v)
{
    return ( m_data == i_v.m_data );
}

inline bool S_Vec2::operator!=(const S_Vec2 &i_v)
{
    return !(*this == i_v);
}

inline float S_Vec2::length()
{
    return glm::length( m_data );
}

inline S_Vec2& S_Vec2::normalize()
{
    m_data = glm::normalize( m_data );
    return *this;
}

inline void S_Vec2::normalizeOut(S_Vec2 &i_out)
{
    i_out.m_data = glm::normalize( m_data );
}

inline float S_Vec2::dot(const S_Vec2 &i_vec)
{
    return glm::dot( m_data, i_vec.m_data );
}

inline void S_Vec2::lerp(const S_Vec2 &i_second, float i_amount )
{
    m_data = glm::mix( m_data, i_second.m_data, i_amount );
}

inline void S_Vec2::lerpOut(S_Vec2 &i_out, const S_Vec2 &i_second, float i_amount )
{
    i_out.m_data = glm::mix( m_data, i_second.m_data, i_amount );
}

