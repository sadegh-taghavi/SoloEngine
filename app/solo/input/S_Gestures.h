#pragma once
#include <unordered_map>

namespace solo
{

// Per-frame gesture state computed from raw pointers (touch points, and
// optionally the mouse as a single pointer for desktop testing).
struct S_GestureState
{
    bool  tapped       = false;  float tapX = 0,    tapY = 0;
    bool  longPressed  = false;  float longX = 0,   longY = 0;
    bool  dragging     = false;  float dragDX = 0,  dragDY = 0;
    bool  pinching     = false;  float pinchDelta = 0; // change in finger distance this frame, pixels
    float pinchCenterX = 0,      pinchCenterY = 0;
};

class S_GestureRecognizer
{
public:
    // raw pointer feed (any id; mouse uses id -1)
    void pointerDown(int id, float x, float y);
    void pointerMove(int id, float x, float y);
    void pointerUp(int id, float x, float y);

    void update(float dtSeconds); // once per frame, before gameplay reads state()
    const S_GestureState& state() const { return m_state; }

private:
    static constexpr float kMoveThreshold  = 12.0f;  // px before a press becomes a drag
    static constexpr float kTapMaxTime     = 0.35f;  // s
    static constexpr float kLongPressTime  = 0.6f;   // s

    struct Pointer
    {
        float startX = 0, startY = 0;
        float x = 0, y = 0;
        float prevX = 0, prevY = 0;
        float time  = 0;
        bool  moved = false;
        bool  longFired = false;
    };

    std::unordered_map<int, Pointer> m_pointers;
    S_GestureState m_state;
    bool  m_pendingTap = false;       float m_pendingTapX = 0, m_pendingTapY = 0;
    float m_prevPinchDistance = -1.0f;
};

}
