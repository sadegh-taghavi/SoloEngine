#pragma once
#include <functional>
#include <chrono>
#include <stdint.h>
#include "solo/stl/S_Array.h"
#include "solo/math/S_Math.h"
#include "solo/application/S_Application.h"
#include "S_Animation.h"

namespace solo
{


template<class T>
class S_Interpolate: public S_AnimationBase<T>
{

public:
    S_Interpolate(): S_AnimationBase<T>(),
        m_easingType{ S_EasingType::CubicIn, S_EasingType::LinearInterpolation, S_EasingType::BackOut },
        m_duration{ 300.0f, 100.0f, 500.0f },
        m_enable( true ), m_repeatedTo( false ), m_easeState( 0 )
    {

    }

    virtual ~S_Interpolate(){}

    virtual bool enable() const
    {
        return m_enable;
    }

    virtual void setEnable(bool enable)
    {
        m_enable = enable;
    }

    virtual void setTo(T to)
    {
        if( m_enable )
        {
            S_AnimationBase<T>::setTo( to );
        }else
        {
            S_AnimationBase<T>::m_from = to;
            S_AnimationBase<T>::m_current = to;
        }
    }

    virtual void update()
    {
        if( !m_enable )
            return;

        float dx = S_AnimationBase<T>::m_to.x() - S_AnimationBase<T>::m_current.x();
        float dy = S_AnimationBase<T>::m_to.y() - S_AnimationBase<T>::m_current.y();
        float dz = S_AnimationBase<T>::m_to.z() - S_AnimationBase<T>::m_current.z();
        s_debug("DXXXXX", dx, "DYYYYY", dy, "DZZZZZ", dz );
        if( abs(dx) > 0.01f )
            S_AnimationBase<T>::m_current.setX( S_AnimationBase<T>::m_current.x() +
                                                dx * (1000.0f / 10.0f * S_Application::executingApplication()->renderer()->elapsedTimeUs() / 1000000.0f) );

        if( abs(dy) > 0.01f )
            S_AnimationBase<T>::m_current.setY( S_AnimationBase<T>::m_current.y() +
                                                dy * (1000.0f / 10.0f * S_Application::executingApplication()->renderer()->elapsedTimeUs() / 1000000.0f) );

        if( abs(dz) > 0.01f )
            S_AnimationBase<T>::m_current.setZ( S_AnimationBase<T>::m_current.z() +
                                                dz * (1000.0f / 10.0f * S_Application::executingApplication()->renderer()->elapsedTimeUs() / 1000000.0f) );

//        if( S_AnimationBase<T>::m_playState == S_AnimationPlayState::Playing )
//        {
//            S_AnimationBase<T>::m_time += 1000.0f / m_duration[m_easeState] * S_Application::executingApplication()->renderer()->elapsedTimeUs() / 1000000.0f;

//            if( S_AnimationBase<T>::m_time >= 1.0f )
//            {
//                S_AnimationBase<T>::m_time = 1.0f;
//                S_AnimationBase<T>::m_playState = S_AnimationPlayState::Stoped;
//            }

//            S_AnimationBase<T>::m_from.lerpOut( S_AnimationBase<T>::m_current, S_AnimationBase<T>::m_to,
//                                                S_AnimationBase<T>::m_easingFunctions[ static_cast<uint32_t>(m_easingType[m_easeState]) ](S_AnimationBase<T>::m_time) );

//            if( S_AnimationBase<T>::m_playState == S_AnimationPlayState::Stoped )
//            {
//                if( m_easeState == 0 )
//                {
//                    S_AnimationBase<T>::setFrom( S_AnimationBase<T>::m_current );
//                    S_AnimationBase<T>::setTo( m_bufferTo );
//                    S_AnimationBase<T>::m_time = 0.0f;
//                    S_AnimationBase<T>::m_playState = S_AnimationPlayState::Playing;
//                    m_easeState = m_repeatedTo ? 1 : 2;
//                }else if( m_easeState == 1 && m_easeState == 1 && !m_repeatedTo )
//                {
//                    S_AnimationBase<T>::setFrom( S_AnimationBase<T>::m_current );
//                    S_AnimationBase<T>::setTo( m_bufferTo );
//                    S_AnimationBase<T>::m_time = 0.0;
//                    S_AnimationBase<T>::m_playState = S_AnimationPlayState::Playing;
//                    m_easeState = 2;
//                }
//                else if( m_easeState == 2 )
//                    m_easeState = 0;
//            }
//            m_repeatedTo = false;
//        }

    }

    virtual float durationStart() const
    {
        return m_duration[0];
    }

    virtual void setDurationStart(float durationStart)
    {
        m_duration[0] = durationStart;
    }

    virtual float durationRepeat() const
    {
        return m_duration[1];
    }

    virtual void setDurationRepeat(float durationRepeat)
    {
        m_duration[1] = durationRepeat;
    }

    virtual float durationEnd() const
    {
        return m_duration[2];
    }

    virtual void setDurationEnd(float durationEnd)
    {
        m_duration[2] = durationEnd;
    }

    virtual S_EasingType easingTypeStart() const
    {
        return m_easingType[0];
    }

    virtual void setEasingTypeStart(const S_EasingType &easingTypeStart)
    {
        m_easingType[0] = easingTypeStart;
    }

    virtual S_EasingType easingTypeRepeat() const
    {
        return m_easingType[1];
    }

    virtual void setEasingTypeRepeat(const S_EasingType &easingTypeRepeat)
    {
        m_easingType[1] = easingTypeRepeat;
    }

    virtual S_EasingType easingTypeEnd() const
    {
        return m_easingType[2];
    }

    virtual void setEasingTypeEnd(const S_EasingType &easingTypeEnd)
    {
        m_easingType[2] = easingTypeEnd;
    }

protected:
    S_Array<S_EasingType, 3> m_easingType;
    S_Array<float, 3> m_duration;
    bool m_enable;
    bool m_repeatedTo;
    uint32_t m_easeState;
    T m_bufferTo;

};



}
