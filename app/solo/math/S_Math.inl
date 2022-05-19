#pragma once
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/constants.hpp>
#include <glm/gtx/easing.hpp>
#include "S_Vec2.h"
#include "S_Vec3.h"
#include "S_Vec4.h"

namespace solo
{

inline float min(float v1, float v2)
{
    return glm::min(v1, v2);
}

inline float max(float v1, float v2)
{
    return glm::max(v1, v2);
}

inline double min(double v1, double v2)
{
    return glm::min(v1, v2);
}

inline double max(double v1, double v2)
{
    return glm::max(v1, v2);
}

inline unsigned int min(unsigned int v1, unsigned int v2)
{
    return glm::min(v1, v2);
}

inline unsigned int max(unsigned int v1, unsigned int v2)
{
    return glm::max(v1, v2);
}

inline int min(int v1, int v2)
{
    return glm::min(v1, v2);
}

inline int max(int v1, int v2)
{
    return glm::max(v1, v2);
}

inline S_Vec2 min(S_Vec2 v1, S_Vec2 v2)
{
    return S_Vec2( &glm::min(v1.m_data, v2.m_data)[0] );
}

inline S_Vec2 max(S_Vec2 v1, S_Vec2 v2)
{
    return S_Vec2( &glm::max(v1.m_data, v2.m_data)[0] );
}

inline S_Vec3 min(S_Vec3 v1, S_Vec3 v2)
{
    return S_Vec3( &glm::min(v1.m_data, v2.m_data)[0] );
}

inline S_Vec3 max(S_Vec3 v1, S_Vec3 v2)
{
    return S_Vec3( &glm::max(v1.m_data, v2.m_data)[0] );
}

inline S_Vec4 min(S_Vec4 v1, S_Vec4 v2)
{
    return S_Vec4( &glm::min(v1.m_data, v2.m_data)[0] );
}

inline S_Vec4 max(S_Vec4 v1, S_Vec4 v2)
{
    return S_Vec4( &glm::max(v1.m_data, v2.m_data)[0] );
}

template<typename T>
inline T radians(T v)
{
    return glm::radians(v);
}

template<typename T>
inline T degree(T v)
{
    return glm::degrees(v);
}

template<typename T>
inline T cos(T v)
{
    return glm::cos(v);
}

template<typename T>
inline T sin(T v)
{
    return glm::sin(v);
}

template<typename T>
inline T pow(T v1, T v2)
{
    return glm::pow(v1, v2);
}

template<typename T>
inline T sqrt(T v)
{
    return glm::sqrt(v);
}

template<typename T>
inline T lerp(T v1, T v2, float amount)
{
    return glm::mix(v1, v2, amount);
}

constexpr inline float pI()
{
    return glm::pi<float>();
}

template<typename T>
inline T easeLinearInterpolation( T x )
{
    return glm::linearInterpolation(x);
}

template<typename T>
inline T easeQuadraticIn( T x )
{
    return glm::quadraticEaseIn(x);
}

template<typename T>
inline T easeQuadraticOut( T x )
{
    return glm::quadraticEaseOut(x);
}

template<typename T>
inline T easeQuadraticInOut( T x )
{
    return glm::quadraticEaseInOut(x);
}

template<typename T>
inline T easeCubicIn( T x )
{
    return glm::cubicEaseIn(x);
}

template<typename T>
inline T easeCubicOut( T x )
{
    return glm::cubicEaseOut(x);
}

template<typename T>
inline T easeCubicInOut( T x )
{
    return glm::cubicEaseInOut(x);
}

template<typename T>
inline T easeQuarticIn( T x )
{
    return glm::quarticEaseIn(x);
}

template<typename T>
inline T easeQuarticOut( T x )
{
    return glm::quarticEaseOut(x);
}

template<typename T>
inline T easeQuarticInOut( T x )
{
    return glm::quarticEaseInOut(x);
}

template<typename T>
inline T easeQuinticIn( T x )
{
    return glm::quinticEaseIn(x);
}

template<typename T>
inline T easeQuinticOut( T x )
{
    return glm::quinticEaseOut(x);
}

template<typename T>
inline T easeQuinticInOut( T x )
{
    return glm::quinticEaseInOut(x);
}

template<typename T>
inline T easeSineIn( T x )
{
    return glm::sineEaseIn(x);
}

template<typename T>
inline T easeSineOut( T x )
{
    return glm::sineEaseOut(x);
}

template<typename T>
inline T easeSineInOut( T x )
{
    return glm::sineEaseInOut(x);
}

template<typename T>
inline T easeCircularIn( T x )
{
    return glm::circularEaseIn(x);
}

template<typename T>
inline T easeCircularOut( T x )
{
    return glm::circularEaseOut(x);
}

template<typename T>
inline T easeCircularInOut( T x )
{
    return glm::circularEaseInOut(x);
}

template<typename T>
inline T easeExponentialIn( T x )
{
    return glm::exponentialEaseIn(x);
}

template<typename T>
inline T easeExponentialOut( T x )
{
    return glm::exponentialEaseOut(x);
}

template<typename T>
inline T easeExponentialInOut( T x )
{
    return glm::exponentialEaseInOut(x);
}

template<typename T>
inline T easeElasticIn( T x )
{
    return glm::elasticEaseIn(x);
}

template<typename T>
inline T easeElasticOut( T x )
{
    return glm::elasticEaseOut(x);
}

template<typename T>
inline T easeElasticInOut( T x )
{
    return glm::elasticEaseInOut(x);
}

template<typename T>
inline T easeBackEaseIn( T x )
{
    return glm::backEaseIn(x);
}

template<typename T>
inline T easeBackOut( T x )
{
    return glm::backEaseOut(x);
}

template<typename T>
inline T easeBackInOut( T x )
{
    return glm::backEaseInOut(x);
}

template<typename T>
inline T easeBackIn( T x, T o )
{
    return glm::backEaseIn(x, o);
}

template<typename T>
inline T easeBackIn( T x )
{
    return glm::backEaseIn(x);
}

template<typename T>
inline T easeBackOut( T x, T o )
{
    return glm::backEaseOut(x, o);
}

template<typename T>
inline T easeBackEaseInOut( T x, T o )
{
    return glm::backEaseInOut(x, o);
}

template<typename T>
inline T easeBackEaseInOut( T x )
{
    return glm::backEaseInOut(x);
}

template<typename T>
inline T easeBounceIn( T x )
{
    return glm::bounceEaseIn(x);
}

template<typename T>
inline T easeBounceOut( T x )
{
    return glm::bounceEaseOut(x);
}

template<typename T>
inline T easeBounceInOut( T x )
{
    return glm::bounceEaseInOut(x);
}




//inline float easeInSine( float x )
//{
//    return 1.0f - cos((x * pI()) / 2.0f);
//}

//inline float easeOutSine( float x )
//{
//    return sin((x * pI()) / 2.0f);
//}

//inline float easeInOutSine( float x )
//{
//    return -(cos(pI() * x) - 1.0f) / 2.0f;
//}

//inline float easeInQuad( float x )
//{
//    return x * x;
//}

//inline float easeOutQuad( float x )
//{
//    return 1.0f - (1.0f - x) * (1.0f - x);
//}

//inline float easeInOutQuad( float x )
//{
//    return x < 0.5f ? 2.0f * x * x : 1.0f - pow(-2.0f * x + 2.0f, 2.0f) / 2.0f;
//}

//inline float easeInCubic( float x )
//{
//    return x * x * x;
//}

//inline float easeOutCubic( float x )
//{
//    return 1.0f - pow(1.0f - x, 3.0f);
//}

//inline float easeInOutCubic( float x )
//{
//    return x < 0.5f ? 4.0f * x * x * x : 1.0f - pow(-2.0f * x + 2.0f, 3.0f) / 2.0f;
//}

//inline float easeInQuart( float x )
//{
//    return x * x * x * x;
//}

//inline float easeOutQuart( float x )
//{
//    return 1.0f - pow(1.0f - x, 4.0f);
//}

//inline float easeInOutQuart( float x )
//{
//    return x < 0.5f ? 8.0f * x * x * x * x : 1.0f - pow(-2.0f * x + 2.0f, 4.0f) / 2.0f;
//}

//inline float easeInQuint( float x )
//{
//    return x * x * x * x * x;
//}

//inline float easeOutQuint( float x )
//{
//    return 1.0f - pow(1.0f - x, 5.0f);
//}

//inline float easeInOutQuint( float x )
//{
//    return x < 0.5f ? 16.0f * x * x * x * x * x : 1.0f - pow(-2.0f * x + 2.0f, 5.0f) / 2.0f;
//}

//inline float easeInExpo( float x )
//{
//    return x == 0.0f ? 0.0f : pow(2.0f, 10.0f * x - 10.0f);
//}

//inline float easeOutExpo( float x )
//{
//    return x == 1.0f ? 1.0f : 1.0f - pow(2.0f, -10.0f * x);
//}

//inline float easeInOutExpo( float x )
//{
//    return x == 0.0f
//          ? 0.0f
//          : x == 1.0f
//          ? 1.0f
//          : x < 0.5f ? pow(2.0f, 20.0f * x - 10.0f) / 2.0f
//          : (2.0f - pow(2.0f, -20.0f * x + 10.0f)) / 2.0f;
//}

//inline float easeInCirc( float x )
//{
//    return 1.0f - sqrt(1.0f - pow(x, 2.0f));
//}

//inline float easeOutCirc( float x )
//{
//    return sqrt(1.0f - pow(x - 1.0f, 2.0f));
//}

//inline float easeInOutCirc( float x )
//{
//    return x < 0.5f
//          ? (1.0f - sqrt(1.0f - pow(2.0f * x, 2.0f))) / 2.0f
//          : (sqrt(1.0f - pow(-2.0f * x + 2.0f, 2.0f)) + 1.0f) / 2.0f;
//}

//inline float easeInBack( float x )
//{
//    static const float c1 = 1.70158f;
//    static const float c3 = c1 + 1.0f;
//    return c3 * x * x * x - c1 * x * x;
//}

//inline float easeOutBack( float x )
//{
//    static const float c1 = 1.70158f;
//    static const float c3 = c1 + 1.0f;
//    return 1.0f + c3 * pow(x - 1.0f, 3.0f) + c1 * pow(x - 1.0f, 2.0f);
//}

//inline float easeInOutBack( float x )
//{
//    static const float c1 = 1.70158f;
//    static const float c2 = c1 * 1.525f;
//    return x < 0.5f
//          ? (pow(2.0f * x, 2.0f) * ((c2 + 1) * 2.0f * x - c2)) / 2.0f
//          : (pow(2.0f * x - 2.0f, 2.0f) * ((c2 + 1.0f) * (x * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
//}

//inline float easeInElastic( float x )
//{
//    static const float c4 = (2.0f * pI()) / 3.0f;
//    return x == 0.0f
//          ? 0.0f
//          : x == 1.0f
//          ? 1.0f
//          : -pow(2.0f, 10.0f * x - 10.0f) * sin((x * 10.0f - 10.75f) * c4);
//}

//inline float easeOutElastic( float x )
//{
//    static const float c4 = (2 * pI()) / 3.0f;
//    return x == 0.0f
//          ? 0.0f
//          : x == 1.0f
//          ? 1.0f
//          : pow(2.0f, -10.0f * x) * sin((x * 10.0f - 0.75f) * c4) + 1.0f;
//}

//inline float easeInOutElastic( float x )
//{
//    static const float c5 = (2.0f * pI()) / 4.5f;
//    return x == 0.0f
//          ? 0.0f
//          : x == 1.0f
//          ? 1.0f
//          : x < 0.5f
//          ? -(pow(2.0f, 20.0f * x - 10.0f) * sin((20.0f * x - 11.125f) * c5)) / 2.0f
//          : (pow(2.0f, -20.0f * x + 10.0f) * sin((20.0f * x - 11.125f) * c5)) / 2.0f + 1.0f;
//}

//inline float easeOutBounce( float x )
//{
//    static const float n1 = 7.5625f;
//    static const float d1 = 2.75f;
//    if (x < 1.0f / d1)
//    {
//        return n1 * x * x;
//    }else if (x < 2.0f / d1)
//    {
//        x -= 1.5f / d1;
//        return n1 * x * x + 0.75f;
//    }else if (x < 2.5f / d1) {
//        x -= 2.25f / d1;
//        return n1 * x * x + 0.9375f;
//    }else {
//        x -= 2.625f / d1;
//        return n1 * x * x + 0.984375f;
//    }
//}

//inline float easeInBounce( float x )
//{
//    return 1.0f - easeOutBounce(1.0f - x);
//}

//inline float easeInOutBounce( float x )
//{
//    return x < 0.5f ?
//                (1.0f - easeOutBounce(1.0f - 2.0f * x)) / 2.0f :
//                      (1.0f + easeOutBounce(2.0f * x - 1.0f)) / 2.0f;
//}

}
