#include "S_Gamepad.h"
#include "solo/platforms/S_SystemDetect.h"
#include <cmath>

using namespace solo;

#ifdef S_PLATFORM_WINDOWS

#define NOMINMAX
#include <windows.h>
#include <xinput.h>

static void applyRadialDeadZone(float rawX, float rawY, float deadZone, float& outX, float& outY)
{
    const float len = std::sqrt(rawX * rawX + rawY * rawY);
    if (len <= deadZone)
    {
        outX = 0.0f;
        outY = 0.0f;
        return;
    }
    const float scaled = (std::fmin(len, 1.0f) - deadZone) / (1.0f - deadZone);
    outX = rawX / len * scaled;
    outY = rawY / len * scaled;
}

void S_Gamepad::poll()
{
    XINPUT_STATE xs{};
    if (XInputGetState(0, &xs) != ERROR_SUCCESS)
    {
        m_state = S_GamepadState{};
        return;
    }

    m_state.connected = true;
    const WORD b = xs.Gamepad.wButtons;
    auto set = [this](S_GamepadButton btn, bool down) { m_state.buttons[static_cast<uint32_t>(btn)] = down; };
    set(S_GamepadButton::A,             b & XINPUT_GAMEPAD_A);
    set(S_GamepadButton::B,             b & XINPUT_GAMEPAD_B);
    set(S_GamepadButton::X,             b & XINPUT_GAMEPAD_X);
    set(S_GamepadButton::Y,             b & XINPUT_GAMEPAD_Y);
    set(S_GamepadButton::DPadUp,        b & XINPUT_GAMEPAD_DPAD_UP);
    set(S_GamepadButton::DPadDown,      b & XINPUT_GAMEPAD_DPAD_DOWN);
    set(S_GamepadButton::DPadLeft,      b & XINPUT_GAMEPAD_DPAD_LEFT);
    set(S_GamepadButton::DPadRight,     b & XINPUT_GAMEPAD_DPAD_RIGHT);
    set(S_GamepadButton::LeftShoulder,  b & XINPUT_GAMEPAD_LEFT_SHOULDER);
    set(S_GamepadButton::RightShoulder, b & XINPUT_GAMEPAD_RIGHT_SHOULDER);
    set(S_GamepadButton::LeftThumb,     b & XINPUT_GAMEPAD_LEFT_THUMB);
    set(S_GamepadButton::RightThumb,    b & XINPUT_GAMEPAD_RIGHT_THUMB);
    set(S_GamepadButton::Start,         b & XINPUT_GAMEPAD_START);
    set(S_GamepadButton::Back,          b & XINPUT_GAMEPAD_BACK);

    constexpr float kStickDeadZone   = 0.24f; // ~XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767
    constexpr float kTriggerDeadZone = 30.0f / 255.0f;

    float lx, ly, rx, ry;
    applyRadialDeadZone(xs.Gamepad.sThumbLX / 32767.0f, xs.Gamepad.sThumbLY / 32767.0f, kStickDeadZone, lx, ly);
    applyRadialDeadZone(xs.Gamepad.sThumbRX / 32767.0f, xs.Gamepad.sThumbRY / 32767.0f, kStickDeadZone, rx, ry);
    m_state.axes[static_cast<uint32_t>(S_GamepadAxis::LeftX)]  = lx;
    m_state.axes[static_cast<uint32_t>(S_GamepadAxis::LeftY)]  = ly;
    m_state.axes[static_cast<uint32_t>(S_GamepadAxis::RightX)] = rx;
    m_state.axes[static_cast<uint32_t>(S_GamepadAxis::RightY)] = ry;

    auto trigger = [](BYTE v, float dz) { float f = v / 255.0f; return f <= dz ? 0.0f : (f - dz) / (1.0f - dz); };
    m_state.axes[static_cast<uint32_t>(S_GamepadAxis::LeftTrigger)]  = trigger(xs.Gamepad.bLeftTrigger,  kTriggerDeadZone);
    m_state.axes[static_cast<uint32_t>(S_GamepadAxis::RightTrigger)] = trigger(xs.Gamepad.bRightTrigger, kTriggerDeadZone);
}

#else

void S_Gamepad::poll()
{
    // Android GameController backend goes here; disconnected state until then
    m_state = S_GamepadState{};
}

#endif
