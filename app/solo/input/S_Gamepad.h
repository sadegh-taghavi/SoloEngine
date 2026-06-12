#pragma once
#include <cstdint>

namespace solo
{

enum class S_GamepadButton : uint32_t
{
    A = 0, B, X, Y,
    DPadUp, DPadDown, DPadLeft, DPadRight,
    LeftShoulder, RightShoulder,
    LeftThumb, RightThumb,
    Start, Back,
    Count
};

enum class S_GamepadAxis : uint32_t
{
    LeftX = 0, LeftY, RightX, RightY,
    LeftTrigger, RightTrigger,
    Count
};

struct S_GamepadState
{
    bool  connected = false;
    bool  buttons[static_cast<uint32_t>(S_GamepadButton::Count)] = {};
    float axes[static_cast<uint32_t>(S_GamepadAxis::Count)]      = {};

    bool  button(S_GamepadButton b) const { return buttons[static_cast<uint32_t>(b)]; }
    float axis(S_GamepadAxis a)     const { return axes[static_cast<uint32_t>(a)]; }
};

// Polls the platform gamepad API once per frame (XInput on Windows; the
// Android GameController backend plugs into the same interface later).
// Sticks get a radial dead zone, triggers a linear one.
class S_Gamepad
{
public:
    void poll(); // platform-specific; no-op where unsupported
    const S_GamepadState& state() const { return m_state; }

private:
    S_GamepadState m_state;
};

}
