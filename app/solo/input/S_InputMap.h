#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "S_Gamepad.h"

namespace solo
{

class S_InputState;

// Action mapping layer: gameplay reads named actions/axes instead of raw keys,
// so the same code runs on desktop (keys/mouse) and mobile (virtual controls).
// Bindings come from a JSON file in the pack:
//   { "actions": { "Jump": ["Key.Space", "Mouse.Left"] },
//     "axes":    { "MoveX": [{ "pos": "Key.D", "neg": "Key.A" }] } }
// Virtual inputs (touch joystick, on-screen buttons) are pushed in by code via
// setVirtualButton/setVirtualAxis and combine with the physical bindings.
class S_InputMap
{
public:
    bool load(const std::string& packPath);
    void update(const S_InputState& state, const S_GamepadState& pad = S_GamepadState()); // once per frame

    bool  action(const std::string& name) const;        // currently held
    bool  actionPressed(const std::string& name) const; // went down this frame
    bool  actionReleased(const std::string& name) const;
    float axis(const std::string& name) const;          // clamped [-1, 1]

    void setVirtualButton(const std::string& action, bool down);
    void setVirtualAxis(const std::string& axisName, float value);

private:
    struct Binding
    {
        int key         = -1; // S_Key
        int mouseButton = -1; // S_MouseButton
        int padButton   = -1; // S_GamepadButton
    };
    struct ActionState
    {
        std::vector<Binding> bindings;
        bool down = false, prev = false, virtualDown = false;
    };
    struct AxisBinding
    {
        Binding positive, negative;
        int     padAxis = -1; // S_GamepadAxis, used directly when >= 0
        float   padScale = 1.0f;
    };
    struct AxisState
    {
        std::vector<AxisBinding> bindings;
        float value = 0.0f, virtualValue = 0.0f;
    };

    static Binding parseBinding(const std::string& name);

    std::unordered_map<std::string, ActionState> m_actions;
    std::unordered_map<std::string, AxisState>   m_axes;
};

}
