#include "S_Vec3.h"
#include "S_Mat4x4.h"

inline S_Vec3::S_Vec3( float i_x, float i_y, float i_z ) : m_data( i_x, i_y, i_z )
{
}

inline S_Vec3::S_Vec3( const float *i_array ) : m_data( i_array[0], i_array[1], i_array[2] )
{
}

inline S_Vec3::S_Vec3( const S_Vec3 &i_vec )
{
	*this = i_vec;
}

inline float S_Vec3::x()
{
    return m_data.x;
}

inline void S_Vec3::setX(float x)
{
    m_data[0] = x;
}

inline float S_Vec3::y()
{
    return m_data.y;
}

inline void S_Vec3::setY(float y)
{
    m_data.y = y;
}

inline float S_Vec3::z()
{
    return m_data.z;
}

inline void S_Vec3::setZ(float z)
{
    m_data.z = z;
}

inline S_Vec3 S_Vec3::operator+( const S_Vec3 &i_v )
{
    S_Vec3 ret;
    ret.m_data = m_data + i_v.m_data;
    return ret;
}

inline S_Vec3& S_Vec3::operator+=( const S_Vec3 &i_v )
{
    m_data += i_v.m_data;
    return *this;
}

inline S_Vec3 S_Vec3::operator-( )
{
    S_Vec3 ret;
    ret.m_data = -m_data;
    return ret;
}

inline S_Vec3 S_Vec3::operator-( const S_Vec3 &i_v )
{
    S_Vec3 ret;
    ret.m_data = m_data - i_v.m_data;
    return ret;
}

inline S_Vec3& S_Vec3::operator-=( const S_Vec3 &i_v )
{
    m_data -= i_v.m_data;
    return *this;
}

inline S_Vec3 S_Vec3::operator*( float i_scaler )
{
    S_Vec3 ret;
    ret.m_data = m_data * i_scaler;
    return ret;
}

inline S_Vec3& S_Vec3::operator*=( float i_scaler )
{
    m_data *= i_scaler;
    return *this;
}

inline S_Vec3 S_Vec3::operator/( float i_scaler )
{
    S_Vec3 ret;
    ret.m_data = m_data / i_scaler;
    return ret;
}

inline S_Vec3& S_Vec3::operator/=( float i_scaler )
{
    m_data /= i_scaler;
    return *this;
}

inline bool S_Vec3::operator==( const S_Vec3 &i_v )
{
    if( m_data == i_v.m_data )
        return false;
    return true;
}

inline bool S_Vec3::operator!=( const S_Vec3 &i_v )
{
	return !( *this == i_v );
}

inline float S_Vec3::length()
{
    return glm::length( m_data );
}

inline S_Vec3& S_Vec3::normalize()
{
    m_data = glm::normalize( m_data );
    return *this;
}

inline S_Vec3& S_Vec3::normalizeBy( const S_Vec3* i_vec )
{
    m_data = glm::normalize( i_vec->m_data );
	return *this;
}

inline void S_Vec3::normalizeOut( S_Vec3* i_out )
{
    i_out->m_data = glm::normalize( m_data );
}

inline float S_Vec3::dot( const S_Vec3 *i_vec )
{
    return glm::dot( m_data, i_vec->m_data );
}

inline S_Vec3& S_Vec3::cross( const S_Vec3 *i_vec )
{
    m_data = glm::cross( m_data, i_vec->m_data );
    return *this;
}

inline void S_Vec3::crossOut( S_Vec3 *i_out, const S_Vec3 *i_vec )
{
    i_out->m_data = glm::cross( m_data, i_vec->m_data );
}

inline void S_Vec3::lerp( const S_Vec3 *i_second, float i_amount )
{
    m_data = glm::mix( m_data, i_second->m_data, i_amount );
}

inline void S_Vec3::lerpOut( S_Vec3 *i_out, const S_Vec3 *i_second, float i_amount )
{
    i_out->m_data = glm::mix( m_data, i_second->m_data, i_amount );
}

//inline bool S_Vec3RayIntersectToPlane( const S_Vec3 *i_org, const S_Vec3 *i_dir, const S_Vec3 *i_verts, float &i_distance )
//{
//	DirectX::XMVECTOR v[ 4 ];
//	v[ 0 ] = DirectX::XMLoadFloat3( ( DirectX::XMFLOAT3 * )&i_verts[ 0 ] );
//	v[ 1 ] = DirectX::XMLoadFloat3( ( DirectX::XMFLOAT3 * )&i_verts[ 1 ] );
//	v[ 2 ] = DirectX::XMLoadFloat3( ( DirectX::XMFLOAT3 * )&i_verts[ 2 ] );
//	v[ 3 ] = DirectX::XMLoadFloat3( ( DirectX::XMFLOAT3 * )&i_verts[ 3 ] );

//	if ( DirectX::TriangleTests::Intersects( DirectX::XMLoadFloat3( ( DirectX::XMFLOAT3 * )i_org ),
//		DirectX::XMLoadFloat3( ( DirectX::XMFLOAT3 * )i_dir ), v[ 0 ], v[ 1 ], v[ 3 ], i_distance ) )
//		return true;

//	if ( DirectX::TriangleTests::Intersects( DirectX::XMLoadFloat3( ( DirectX::XMFLOAT3 * )i_org ),
//		DirectX::XMLoadFloat3( ( DirectX::XMFLOAT3 * )i_dir ), v[ 0 ], v[ 2 ], v[ 3 ], i_distance ) )
//		return true;

//	return false;
//}

//inline bool S_Vec3RayIntersectToBox( const S_Vec3 *i_org, const S_Vec3 *i_dir,
//    const S_Vec3 *i_min, const S_Vec3 *i_max, const S_Mat4x4 *i_transform, float &i_distance )
//{
//	DirectX::BoundingBox bb;
//	DirectX::BoundingBox::CreateFromPoints( bb, DirectX::XMLoadFloat3( ( DirectX::XMFLOAT3 * )i_min ),
//		DirectX::XMLoadFloat3( ( DirectX::XMFLOAT3 * )i_max ) );
//	bb.Transform( bb, DirectX::XMLoadFloat4x4( ( DirectX::XMFLOAT4X4 * )i_transform ) );
//	return bb.Intersects( XMLoadFloat3( ( DirectX::XMFLOAT3 * )i_org ), XMLoadFloat3( ( DirectX::XMFLOAT3 * )i_dir ), i_distance );
//}
