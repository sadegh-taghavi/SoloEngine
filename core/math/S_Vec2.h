#pragma once
#include <glm/glm.hpp>

class S_Mat4x4;

class S_Vec2
{
    friend class S_Vec2;
    friend class S_Vec3;
    friend class S_Vec4;
    friend class S_Quaternion;
    friend class S_Mat4x4;

    glm::vec2 m_data;
public:
    S_Vec2(){}
    S_Vec2(float i_x, float i_y);
    S_Vec2(const float *i_array);
    S_Vec2(const S_Vec2 &i_vec);

    S_Vec2 operator+(const S_Vec2 &i_v);
    S_Vec2& operator+=(const S_Vec2 &i_v);
    S_Vec2 operator-();
    S_Vec2 operator-(const S_Vec2 &i_v);
    S_Vec2& operator-=(const S_Vec2 &i_v);
    S_Vec2 operator*(float i_scaler);
    S_Vec2& operator*=(float i_scaler);
    S_Vec2 operator/(float i_scaler);
    S_Vec2& operator/=(float i_scaler);
    bool operator==(const S_Vec2 &i_v);
    bool operator!=(const S_Vec2 &i_v);
	float length();
//    S_Vec2& transform(const S_Mat4x4 *i_mat);
//    void transformOut(S_Vec2 *i_out, const S_Mat4x4 *i_mat);
    S_Vec2& normalize();
    void normalizeOut(S_Vec2* i_out);
    float dot(const S_Vec2 *i_vec);
    S_Vec2& cross(const S_Vec2 *i_vec);
    void crossOut(S_Vec2 *i_out, const S_Vec2 *i_vec);
};

//void S_Vec2Transform(S_Vec2 *i_out, const S_Vec2 *i_vec, const S_Mat4x4 *i_mat);
float S_Vec2Length(const S_Vec2 *i_vec);
void S_Vec2Normalize(S_Vec2 *i_out, const S_Vec2 *i_vec);
float S_Vec2Dot(const S_Vec2 *i_v1, const S_Vec2 *i_v2);
void S_Vec2Cross(S_Vec2 *i_out, const S_Vec2 *i_v1, const S_Vec2 *i_v2);

#include "S_Vec2.inl"
