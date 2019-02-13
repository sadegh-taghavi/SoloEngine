//#include "S_Mat4x4.h"

//inline S_Mat4x4::S_Mat4x4(	float i_00, float i_01, float i_02, float i_03,
//						float i_10, float i_11, float i_12, float i_13,
//						float i_20, float i_21, float i_22, float i_23,
//						float i_30, float i_31, float i_32, float i_33) :
//	_00(i_00), _01(i_01), _02(i_02), _03(i_03),
//	_10(i_10), _11(i_11), _12(i_12), _13(i_13),
//	_20(i_20), _21(i_21), _22(i_22), _23(i_23),
//	_30(i_30), _31(i_31), _32(i_32), _33(i_33)
//{
//}

//inline S_Mat4x4::S_Mat4x4( const float *i_array )
//{
//	memcpy_s(m, sizeof(float) * 16, i_array, sizeof(float) * 16);
//}

//inline S_Mat4x4::S_Mat4x4( const S_Mat4x4 &i_mat44 )
//{
//	*this = i_mat44;
//}

//inline float S_Mat4x4::operator()( size_t i_row, size_t i_column ) const
//{
//	return _m[i_row][i_column];
//}

//inline float& S_Mat4x4::operator()( size_t i_row, size_t i_column )
//{
//	return _m[i_row][i_column];
//}

//inline S_Mat4x4& S_Mat4x4::operator=( const S_Mat4x4& i_mat44 )
//{
//	memcpy_s(m, sizeof(float) * 16, i_mat44.m, sizeof(float) * 16);
//	return *this;
//}

//inline S_Mat4x4 S_Mat4x4::operator*(const S_Mat4x4 &i_mat44)
//{
//    S_Mat4x4 m = *this;
//	m *= i_mat44;
//	return m;
//}

//inline S_Mat4x4& S_Mat4x4::operator*=(const S_Mat4x4 &i_mat44)
//{
//	return multiply(&i_mat44);
//}

//inline S_Mat4x4& S_Mat4x4::multiply( const S_Mat4x4 *i_mat44 )
//{
//	DirectX::XMMATRIX m1 = DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)this);
//	DirectX::XMMATRIX m2 = DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)i_mat44);
//	XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this, m1 * m2);
//	return *this;
//}

//inline void S_Mat4x4::multiplyOut( S_Mat4x4 *i_out, const S_Mat4x4 *i_mat44 )
//{
//	DirectX::XMMATRIX m1 = DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)this);
//	DirectX::XMMATRIX m2 = DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)i_mat44);
//	XMStoreFloat4x4((DirectX::XMFLOAT4X4*)i_out, m1 * m2);
//}

//inline S_Mat4x4& S_Mat4x4::srp( const S_Vec3 &i_position, const S_Quat &i_rotation, const S_Vec3 &i_scale )
//{
//	DirectX::XMMATRIX p = DirectX::XMMatrixTranslation(i_position.x, i_position.y, i_position.z);
//	DirectX::XMMATRIX s = DirectX::XMMatrixScaling(i_scale.x, i_scale.y, i_scale.z);
//	DirectX::XMMATRIX r = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)&i_rotation));
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this, s * r * p );
//	return *this;
//}

//inline S_Mat4x4& S_Mat4x4::spr( const S_Vec3 &i_position, const S_Quat &i_rotation, const S_Vec3 &i_scale )
//{
//	DirectX::XMMATRIX p = DirectX::XMMatrixTranslation( i_position.x, i_position.y, i_position.z );
//	DirectX::XMMATRIX s = DirectX::XMMatrixScaling( i_scale.x, i_scale.y, i_scale.z );
//	DirectX::XMMATRIX r = DirectX::XMMatrixRotationQuaternion( DirectX::XMLoadFloat4( ( DirectX::XMFLOAT4* )&i_rotation ) );
//	DirectX::XMStoreFloat4x4( ( DirectX::XMFLOAT4X4* )this, s * p * r );
//	return *this;
//}

//inline S_Mat4x4& S_Mat4x4::transform( const S_Vec3 &i_p )
//{
//	DirectX::XMStoreFloat4x4( ( DirectX::XMFLOAT4X4* )this, DirectX::XMMatrixTranslation( i_p.x, i_p.y, i_p.z ) );
//	return *this;
//}

//inline void S_Mat4x4::transformOut( S_Mat4x4 *i_out, const S_Vec3 &i_p )
//{
//	DirectX::XMStoreFloat4x4( ( DirectX::XMFLOAT4X4* )i_out, DirectX::XMMatrixTranslation( i_p.x, i_p.y, i_p.z ) );
//}


//inline S_Mat4x4& S_Mat4x4::lookAtRH( const S_Vec3 *i_position, const S_Vec3 *i_target, const S_Vec3 *i_up )
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this,
//		DirectX::XMMatrixLookAtRH(DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)i_position),
//		DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)i_target),
//		DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)i_up)));
//	return *this;
//}

//inline S_Mat4x4& S_Mat4x4::inverse()
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this, DirectX::XMMatrixInverse(0, DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)this)));
//	return *this;
//}

//inline S_Mat4x4& S_Mat4x4::inverseBy( const S_Mat4x4 *i_mat )
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this, DirectX::XMMatrixInverse(0, DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)i_mat)));
//	return *this;
//}

//inline void S_Mat4x4::inverseOut( S_Mat4x4 *i_out )
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)i_out, DirectX::XMMatrixInverse(0, DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)this)));
//}

//inline S_Mat4x4& S_Mat4x4::orthoRH( float i_w, float i_h, float i_near, float i_far )
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this, DirectX::XMMatrixOrthographicRH(i_w, i_h, i_near, i_far));
//	return *this;
//}

//inline S_Mat4x4& S_Mat4x4::orthoCenterRH( float i_wLeft, float i_wRight, float i_hBottom, float i_hTop, float i_near, float i_far )
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this, DirectX::XMMatrixOrthographicOffCenterRH( i_wLeft, i_wRight, i_hBottom, i_hTop, i_near, i_far));
//	return *this;
//}


//inline S_Mat4x4& S_Mat4x4::perspectiveFovRH( float i_fov, float i_aspect, float i_near, float i_far )
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this, DirectX::XMMatrixPerspectiveFovRH(i_fov, i_aspect, i_near, i_far));
//	return *this;
//}

//inline S_Mat4x4& S_Mat4x4::transpose()
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)this)));
//	return *this;
//}

//inline S_Mat4x4& S_Mat4x4::transposeBy( const S_Mat4x4 *i_mat )
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)i_mat)));
//	return *this;
//}

//inline void S_Mat4x4::transposeOut( S_Mat4x4 *i_out )
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)i_out, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)this)));
//}

//inline S_Mat4x4& S_Mat4x4::identity()
//{
//    memset(this, 0, sizeof(S_Mat4x4));
//	_00 = 1.0f;
//	_11 = 1.0f;
//	_22 = 1.0f;
//	_33 = 1.0f;
//	return *this;
//}

//inline S_Mat4x4& S_Mat4x4::RotationQuaternion( const S_Quat *i_q )
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this, DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)i_q)));
//	return *this;
//}

//inline void S_Mat4x4::decompose( S_Vec3 *i_outPosition, S_Quat *i_outRotation, S_Vec3 *i_outScale )
//{
//	DirectX::XMVECTOR pos,rot,scale;
//	DirectX::XMMatrixDecompose( &scale, &rot, &pos, DirectX::XMLoadFloat4x4( (DirectX::XMFLOAT4X4 *) this ) );
//	DirectX::XMStoreFloat3( (DirectX::XMFLOAT3 *) i_outPosition, pos );
//	DirectX::XMStoreFloat3( (DirectX::XMFLOAT3 *) i_outScale, scale );
//	DirectX::XMStoreFloat4( (DirectX::XMFLOAT4 *) i_outRotation, rot );
//}


//inline S_Mat4x4& S_Mat4x4::transformation2D( const S_Vec2 *i_scaleCenter, float i_scalingRotation, const S_Vec2 *i_scale, const S_Vec2 *i_rotationCenter, float i_rotation, const S_Vec2 *i_position )
//{
//	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)this,
//		DirectX::XMMatrixTransformation2D(
//		DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)i_scaleCenter),
//		i_scalingRotation,
//		DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)i_scale),
//		DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)i_rotationCenter),
//		i_rotation,
//		DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)i_position)
//		)
//		);
//	return *this;
//}
