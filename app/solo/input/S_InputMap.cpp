#include "S_InputMap.h"
#include "solo/platforms/S_BaseApplication.h"
#include "solo/application/S_Application.h"
#include "solo/pack/S_Pack.h"
#include "solo/debug/S_Debug.h"
#include "rapidjson/document.h"
#include <algorithm>

using namespace solo;

static int keyFromName(const std::string& n)
{
    if (n.size() == 1)
    {
        char c = n[0];
        if (c >= 'A' && c <= 'Z') return static_cast<int>(S_Key::A) + (c - 'A');
        if (c >= 'a' && c <= 'z') return static_cast<int>(S_Key::A) + (c - 'a');
        if (c >= '1' && c <= '9') return static_cast<int>(S_Key::One) + (c - '1');
        if (c == '0')             return static_cast<int>(S_Key::Zero);
    }
    static const std::unordered_map<std::string, S_Key> named = {
        { "Space",  S_Key::Space },  { "Enter",  S_Key::Enter },  { "Escape", S_Key::Escape },
        { "Tab",    S_Key::Tab },    { "Left",   static_cast<S_Key>(80) }, { "Right", static_cast<S_Key>(79) },
        { "Up",     static_cast<S_Key>(82) },   { "Down",  static_cast<S_Key>(81) },
        { "LeftShift",  S_Key::LeftShift },     { "LeftControl", static_cast<S_Key>(224) },
        { "LeftAlt",    S_Key::LeftAlt },
    };
    auto it = named.find(n);
    return it != named.end() ? static_cast<int>(it->second) : -1;
}

static int padButtonFromName(const std::string& n)
{
    static const std::unordered_map<std::string, S_GamepadButton> named = {
        { "A", S_GamepadButton::A }, { "B", S_GamepadButton::B },
        { "X", S_GamepadButton::X }, { "Y", S_GamepadButton::Y },
        { "DPadUp", S_GamepadButton::DPadUp },       { "DPadDown", S_GamepadButton::DPadDown },
        { "DPadLeft", S_GamepadButton::DPadLeft },   { "DPadRight", S_GamepadButton::DPadRight },
        { "LeftShoulder", S_GamepadButton::LeftShoulder }, { "RightShoulder", S_GamepadButton::RightShoulder },
        { "LeftThumb", S_GamepadButton::LeftThumb }, { "RightThumb", S_GamepadButton::RightThumb },
        { "Start", S_GamepadButton::Start },         { "Back", S_GamepadButton::Back },
    };
    auto it = named.find(n);
    return it != named.end() ? static_cast<int>(it->second) : -1;
}

static int padAxisFromName(const std::string& n)
{
    static const std::unordered_map<std::string, S_GamepadAxis> named = {
        { "LeftX", S_GamepadAxis::LeftX },   { "LeftY", S_GamepadAxis::LeftY },
        { "RightX", S_GamepadAxis::RightX }, { "RightY", S_GamepadAxis::RightY },
        { "LeftTrigger", S_GamepadAxis::LeftTrigger }, { "RightTrigger", S_GamepadAxis::RightTrigger },
    };
    auto it = named.find(n);
    return it != named.end() ? static_cast<int>(it->second) : -1;
}

S_InputMap::Binding S_InputMap::parseBinding(const std::string& name)
{
    Binding b;
    if (name.rfind("Key.", 0) == 0)
        b.key = keyFromName(name.substr(4));
    else if (name.rfind("Mouse.", 0) == 0)
    {
        const std::string btn = name.substr(6);
        if      (btn == "Left")   b.mouseButton = static_cast<int>(S_MouseButton::Left);
        else if (btn == "Right")  b.mouseButton = static_cast<int>(S_MouseButton::Right);
        else if (btn == "Middle") b.mouseButton = static_cast<int>(S_MouseButton::Middle);
    }
    else if (name.rfind("Pad.", 0) == 0)
        b.padButton = padButtonFromName(name.substr(4));
    if (b.key < 0 && b.mouseButton < 0 && b.padButton < 0)
        s_debugLayer("S_InputMap: unknown binding", name);
    return b;
}

bool S_InputMap::load(const std::string& packPath)
{
    auto data = S_Application::executingApplication()->pack()->load(packPath);
    if (data.empty())
    {
        s_debugLayer("S_InputMap: bindings not found", packPath);
        return false;
    }
    data.push_back('\0');

    rapidjson::Document doc;
    doc.Parse(reinterpret_cast<const char*>(data.data()));
    if (doc.HasParseError() || !doc.IsObject())
    {
        s_debugLayer("S_InputMap: bindings JSON parse error", packPath);
        return false;
    }

    if (doc.HasMember("actions") && doc["actions"].IsObject())
        for (auto& m : doc["actions"].GetObject())
        {
            ActionState& a = m_actions[m.name.GetString()];
            if (m.value.IsArray())
                for (auto& b : m.value.GetArray())
                    if (b.IsString()) a.bindings.push_back(parseBinding(b.GetString()));
        }

    if (doc.HasMember("axes") && doc["axes"].IsObject())
        for (auto& m : doc["axes"].GetObject())
        {
            AxisState& a = m_axes[m.name.GetString()];
            if (m.value.IsArray())
                for (auto& b : m.value.GetArray())
                    if (b.IsObject())
                    {
                        AxisBinding ab;
                        if (b.HasMember("pos") && b["pos"].IsString()) ab.positive = parseBinding(b["pos"].GetString());
                        if (b.HasMember("neg") && b["neg"].IsString()) ab.negative = parseBinding(b["neg"].GetString());
                        if (b.HasMember("pad") && b["pad"].IsString()) ab.padAxis  = padAxisFromName(b["pad"].GetString());
                        if (b.HasMember("scale") && b["scale"].IsNumber()) ab.padScale = b["scale"].GetFloat();
                        a.bindings.push_back(ab);
                    }
        }

    s_debugLayer("S_InputMap: loaded", m_actions.size(), "actions,", m_axes.size(), "axes");
    return true;
}

void S_InputMap::update(const S_InputState& state, const S_GamepadState& pad)
{
    auto isDown = [&state, &pad](const Binding& b)
    {
        if (b.key >= 0         && state.isKey(static_cast<S_Key>(b.key)))                 return true;
        if (b.mouseButton >= 0 && state.isKey(static_cast<S_MouseButton>(b.mouseButton))) return true;
        if (b.padButton >= 0   && pad.button(static_cast<S_GamepadButton>(b.padButton)))  return true;
        return false;
    };

    for (auto& kv : m_actions)
    {
        ActionState& a = kv.second;
        a.prev = a.down;
        a.down = a.virtualDown;
        for (const Binding& b : a.bindings)
            if (isDown(b)) { a.down = true; break; }
    }

    for (auto& kv : m_axes)
    {
        AxisState& a = kv.second;
        float v = a.virtualValue;
        for (const AxisBinding& b : a.bindings)
        {
            if (isDown(b.positive)) v += 1.0f;
            if (isDown(b.negative)) v -= 1.0f;
            if (b.padAxis >= 0)
                v += pad.axis(static_cast<S_GamepadAxis>(b.padAxis)) * b.padScale;
        }
        a.value = std::max(-1.0f, std::min(1.0f, v));
    }
}

bool S_InputMap::action(const std::string& name) const
{
    auto it = m_actions.find(name);
    return it != m_actions.end() && it->second.down;
}

bool S_InputMap::actionPressed(const std::string& name) const
{
    auto it = m_actions.find(name);
    return it != m_actions.end() && it->second.down && !it->second.prev;
}

bool S_InputMap::actionReleased(const std::string& name) const
{
    auto it = m_actions.find(name);
    return it != m_actions.end() && !it->second.down && it->second.prev;
}

float S_InputMap::axis(const std::string& name) const
{
    auto it = m_axes.find(name);
    return it != m_axes.end() ? it->second.value : 0.0f;
}

void S_InputMap::setVirtualButton(const std::string& actionName, bool down)
{
    m_actions[actionName].virtualDown = down;
}

void S_InputMap::setVirtualAxis(const std::string& axisName, float value)
{
    m_axes[axisName].virtualValue = value;
}
