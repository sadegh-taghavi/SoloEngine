#include "S_Input.h"
#include <cstring>

using namespace solo;

S_BasicInput::S_BasicInput()
{
    memset( this, 0, sizeof ( S_BasicInput ) );
}

bool S_BasicInput::isKeyDown(S_KeyButton keyButton)
{
    return m_keyboardButtons[ static_cast<int>( keyButton ) ];
}

bool S_BasicInput::isMouseDown(S_MouseButton mouseButton)
{
    return ( m_mouseButtons & static_cast<int>( mouseButton ) ) != 0;
}

int S_BasicInput::mouseX() const
{
    return m_mouseX;
}

int S_BasicInput::mouseY() const
{
    return m_mouseY;
}

int S_BasicInput::mouseDX() const
{
    return m_mouseDX;
}

int S_BasicInput::mouseDY() const
{
    return m_mouseDY;
}

int S_BasicInput::mouseDW() const
{
    return m_mouseDW;
}
