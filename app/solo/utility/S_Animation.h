#pragma once
#include <functional>
#include <chrono>
#include <stdint.h>
#include "solo/stl/S_Array.h"
#include "solo/math/S_Math.h"
#include "solo/application/S_Application.h"

namespace solo
{


enum class S_AnimationPlayState
{
    Playing,
    Paused,
    Stoped
};

enum class S_EasingType
{
    LinearInterpolation,
    QuadraticIn,
    QuadraticOut,
    QuadraticInOut,
    CubicIn,
    CubicOut,
    CubicInOut,
    QuarticIn,
    QuarticOut,
    QuarticInOut,
    QuinticIn,
    QuinticOut,
    QuinticInOut,
    SineIn,
    SineOut,
    SineInOut,
    CircularIn,
    CircularOut,
    CircularInOut,
    ExponentialIn,
    ExponentialOut,
    ExponentialInOut,
    ElasticIn,
    ElasticOut,
    ElasticInOut,
    BackEaseIn,
    BackOut,
    BackInOut,
    BackIn,
    BackEaseInOut,
    BounceIn,
    BounceOut,
    BounceInOut,
    Count
};


template<class T>
class S_AnimationBase
{

public:
     S_AnimationBase():
         m_playState( S_AnimationPlayState::Stoped ),
         m_time( 0.0f )
     {

     }
     virtual ~S_AnimationBase(){}

     virtual float time() const
     {
         return m_time;
     }

     virtual void setTime(float time)
     {
         m_time = min( max( time, 0.0f ), 1.0f );         
     }

     virtual T from() const
     {
         return m_from;
     }

     virtual void setFrom(const T &from)
     {
         m_from = from;
     }

     virtual T to() const
     {
         return m_to;
     }

     virtual void setTo(const T &to)
     {
         m_to = to;
     }

     virtual T current() const
     {
         return m_current;
     }

     virtual void setCurrent(const T &current)
     {
         m_current = current;
     }

     virtual bool isRunning()
     {
         return m_playState == S_AnimationPlayState::Playing;
     }

     virtual S_AnimationPlayState playState() const
     {
         return m_playState;
     }

protected:
    const S_Array<std::function<float(float)>, static_cast<uint32_t>(S_EasingType::Count)> m_easingFunctions =
    {
        [](float x){return easeLinearInterpolation(x);},
        [](float x){return easeQuadraticIn(x);},
        [](float x){return easeQuadraticOut(x);},
        [](float x){return easeQuadraticInOut(x);},
        [](float x){return easeCubicIn(x);},
        [](float x){return easeCubicOut(x);},
        [](float x){return easeCubicInOut(x);},
        [](float x){return easeQuarticIn(x);},
        [](float x){return easeQuarticOut(x);},
        [](float x){return easeQuarticInOut(x);},
        [](float x){return easeQuinticIn(x);},
        [](float x){return easeQuinticOut(x);},
        [](float x){return easeQuinticInOut(x);},
        [](float x){return easeSineIn(x);},
        [](float x){return easeSineOut(x);},
        [](float x){return easeSineInOut(x);},
        [](float x){return easeCircularIn(x);},
        [](float x){return easeCircularOut(x);},
        [](float x){return easeCircularInOut(x);},
        [](float x){return easeExponentialIn(x);},
        [](float x){return easeExponentialOut(x);},
        [](float x){return easeExponentialInOut(x);},
        [](float x){return easeElasticIn(x);},
        [](float x){return easeElasticOut(x);},
        [](float x){return easeElasticInOut(x);},
        [](float x){return easeBackEaseIn(x);},
        [](float x){return easeBackOut(x);},
        [](float x){return easeBackInOut(x);},
        [](float x){return easeBackIn(x);},
        [](float x){return easeBackEaseInOut(x);},
        [](float x){return easeBounceIn(x);},
        [](float x){return easeBounceOut(x);},
        [](float x){return easeBounceInOut(x);},
    };
    S_AnimationPlayState m_playState;
    float m_time;
    T m_from;
    T m_to;
    T m_current;
};



template<class T>
class S_Animation: public S_AnimationBase<T>
{

public:
    S_Animation():
        m_easingType(S_EasingType::LinearInterpolation),
        m_duration( 300.0f ),
        m_loop( 1 ),
        m_currentLoop( 0 )
    {

    }

    virtual ~S_Animation(){}

    virtual void play()
    {
        if( S_AnimationBase<T>::m_playState == S_AnimationPlayState::Stoped )
        {
            S_AnimationBase<T>::m_current = S_AnimationBase<T>::m_from;
            S_AnimationBase<T>::m_time = 0.0f;
            m_currentLoop = 0;
        }
        S_AnimationBase<T>::m_playState = S_AnimationPlayState::Playing;
    }

    virtual void stop()
    {
        S_AnimationBase<T>::m_playState = S_AnimationPlayState::Stoped;
        S_AnimationBase<T>::m_current = S_AnimationBase<T>::m_from;
    }

    virtual void pause()
    {
        S_AnimationBase<T>::m_playState = S_AnimationPlayState::Paused;
    }

    virtual void update()
    {
        if( S_AnimationBase<T>::m_playState != S_AnimationPlayState::Playing )
            return;
        S_AnimationBase<T>::m_time += 1000.0f / m_duration * S_Application::executingApplication()->renderer()->elapsedTimeUs() / 1000000.0f;

        if( S_AnimationBase<T>::m_time >= 1.0f )
        {
            S_AnimationBase<T>::m_time = 1.0f;
            S_AnimationBase<T>::m_playState = S_AnimationPlayState::Stoped;
            if( m_loop > 0 )
                ++m_currentLoop;
        }
        S_AnimationBase<T>::m_from.lerpOut( S_AnimationBase<T>::m_current, S_AnimationBase<T>::m_to
                                            , S_AnimationBase<T>::m_easingFunctions[static_cast<uint32_t>(m_easingType)](S_AnimationBase<T>::m_time) );
        if( S_AnimationBase<T>::m_playState == S_AnimationPlayState::Stoped && ( m_loop < 0 ||  m_currentLoop < m_loop ) )
        {
            S_AnimationBase<T>::m_time = 0.0f;
            S_AnimationBase<T>::m_playState = S_AnimationPlayState::Playing;
        }
    }

    virtual uint32_t loop() const
    {
        return m_loop;
    }

    virtual void setLoop(const uint32_t &loop)
    {
        m_loop = loop;
    }

    virtual float duration() const
    {
        return m_duration;
    }

    virtual void setDuration(float duration)
    {
        m_duration = duration;
    }

    virtual S_EasingType easingType() const
    {
        return m_easingType;
    }

    virtual void setEasingType(const S_EasingType &easingType)
    {
        m_easingType = easingType;
    }

protected:   
    S_EasingType m_easingType;
    float m_duration;
    int32_t m_loop;
    int32_t m_currentLoop;

};



}
