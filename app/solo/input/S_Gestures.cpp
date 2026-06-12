#include "S_Gestures.h"
#include <cmath>

using namespace solo;

void S_GestureRecognizer::pointerDown(int id, float x, float y)
{
    Pointer p;
    p.startX = p.x = p.prevX = x;
    p.startY = p.y = p.prevY = y;
    m_pointers[id] = p;
}

void S_GestureRecognizer::pointerMove(int id, float x, float y)
{
    auto it = m_pointers.find(id);
    if (it == m_pointers.end()) return;
    Pointer& p = it->second;
    p.x = x;
    p.y = y;
    if (std::fabs(x - p.startX) > kMoveThreshold || std::fabs(y - p.startY) > kMoveThreshold)
        p.moved = true;
}

void S_GestureRecognizer::pointerUp(int id, float x, float y)
{
    auto it = m_pointers.find(id);
    if (it == m_pointers.end()) return;
    Pointer& p = it->second;
    if (!p.moved && p.time < kTapMaxTime && !p.longFired)
    {
        m_pendingTap  = true;
        m_pendingTapX = x;
        m_pendingTapY = y;
    }
    m_pointers.erase(it);
}

void S_GestureRecognizer::update(float dtSeconds)
{
    m_state = S_GestureState{};

    if (m_pendingTap)
    {
        m_state.tapped = true;
        m_state.tapX   = m_pendingTapX;
        m_state.tapY   = m_pendingTapY;
        m_pendingTap   = false;
    }

    for (auto& kv : m_pointers)
    {
        Pointer& p = kv.second;
        p.time += dtSeconds;
        if (!p.moved && !p.longFired && p.time >= kLongPressTime)
        {
            p.longFired         = true;
            m_state.longPressed = true;
            m_state.longX       = p.x;
            m_state.longY       = p.y;
        }
    }

    if (m_pointers.size() == 1)
    {
        Pointer& p = m_pointers.begin()->second;
        if (p.moved)
        {
            m_state.dragging = true;
            m_state.dragDX   = p.x - p.prevX;
            m_state.dragDY   = p.y - p.prevY;
        }
        m_prevPinchDistance = -1.0f;
    }
    else if (m_pointers.size() >= 2)
    {
        auto it = m_pointers.begin();
        Pointer& a = (it++)->second;
        Pointer& b = it->second;
        float dx = b.x - a.x, dy = b.y - a.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        m_state.pinching     = true;
        m_state.pinchCenterX = (a.x + b.x) * 0.5f;
        m_state.pinchCenterY = (a.y + b.y) * 0.5f;
        if (m_prevPinchDistance >= 0.0f)
            m_state.pinchDelta = dist - m_prevPinchDistance;
        m_prevPinchDistance = dist;
    }
    else
        m_prevPinchDistance = -1.0f;

    for (auto& kv : m_pointers)
    {
        kv.second.prevX = kv.second.x;
        kv.second.prevY = kv.second.y;
    }
}
