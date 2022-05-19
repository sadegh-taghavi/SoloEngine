#pragma once
#include "S_Event.h"

namespace solo
{

enum class S_Key
{
    None          = 0,
    A             = 4,
    B             = 5,
    C             = 6,
    D             = 7,
    E             = 8,
    F             = 9,
    G             = 10,
    H             = 11,
    I             = 12,
    J             = 13,
    K             = 14,
    L             = 15,
    M             = 16,
    N             = 17,
    O             = 18,
    P             = 19,
    Q             = 20,
    R             = 21,
    S             = 22,
    T             = 23,
    U             = 24,
    V             = 25,
    W             = 26,
    X             = 27,
    Y             = 28,
    Z             = 29,
    One           = 30, // 1 and !
    Two           = 31, // 2 and @
    Three         = 32, // 3 and #
    Four          = 33, // 4 and $
    Five          = 34, // 5 and %
    Six           = 35, // 6 and ^
    Seven         = 36, // 7 and &
    Eight         = 37, // 8 and *
    Nine          = 38, // 9 and (
    Zero          = 39, // 0 and )
    Enter         = 40, // (Return)
    Escape        = 41,
    Delete        = 42,
    Tab           = 43,
    Space         = 44,
    Minus         = 45, // - and (underscore)
    Equals        = 46, // = and +
    LeftBracket   = 47, // [ and {
    RightBracket  = 48, // ] and }
    Backslash     = 49, // \ and |
    NonUSHash     = 50, // # and ~
    Semicolon     = 51, // ; and :
    Quote         = 52, // ' and "
    Grave         = 53,
    Comma         = 54, // , and <
    Period        = 55, // . and >
    Slash         = 56, // / and ?
    CapsLock      = 57,
    F1            = 58,
    F2            = 59,
    F3            = 60,
    F4            = 61,
    F5            = 62,
    F6            = 63,
    F7            = 64,
    F8            = 65,
    F9            = 66,
    F10           = 67,
    F11           = 68,
    F12           = 69,
    PrintScreen   = 70,
    ScrollLock    = 71,
    Pause         = 72,
    Insert        = 73,
    Home          = 74,
    PageUp        = 75,
    DeleteForward = 76,
    End           = 77,
    PageDown      = 78,
    Right         = 79, // Right arrow
    Left          = 80, // Left arrow
    Down          = 81, // Down arrow
    Up            = 82, // Up arrow
    NumLock        = 83,
    PadDivide         = 84,
    PadMultiply       = 85,
    PadSubtract       = 86,
    PadAdd            = 87,
    PadEnter          = 88,
    Pad1              = 89,
    Pad2              = 90,
    Pad3              = 91,
    Pad4              = 92,
    Pad5              = 93,
    Pad6              = 94,
    Pad7              = 95,
    Pad8              = 96,
    Pad9              = 97,
    Pad0              = 98,
    PadPoint          = 99, // . and Del
    PadEquals         = 103,
    F13           = 104,
    F14           = 105,
    F15           = 106,
    F16           = 107,
    F17           = 108,
    F18           = 109,
    F19           = 110,
    F20           = 111,
    F21           = 112,
    F22           = 113,
    F23           = 114,
    F24           = 115,
    Help          = 117,
    Menu          = 118,
    Mute          = 127,
    VolumeUp      = 128,
    VolumeDown    = 129,
    LeftControl   = 224, // WARNING : Android has no Ctrl keys.
    LeftShift     = 225,
    LeftAlt       = 226,
    LeftGUI       = 227,
    RightControl  = 228,
    RightShift    = 229, // WARNING : Win32 fails to send a WM_KEYUP message if both shift keys are pressed, and one released.
    RightAlt      = 230,
    RightGUI      = 231,
    Count
};

enum class S_MouseButton
{
    None    = 0,
    Left    = 1,
    Right   = 2,
    Middle  = 3,
    X1      = 4,
    X2      = 5,
    Count
};

enum class S_KeyboardEventState
{
    Up = 0,
    Down = 1,
};

enum class S_MouseEventState
{
    Up = 0,
    Down = 1,
    Move = 2,
};

enum class S_TouchEventState
{
    Up = 0,
    Down = 1,
    Move = 2,
};

class S_KeyboardEvent : public S_Event
{
public:
    S_KeyboardEvent(const S_Key &key, const S_KeyboardEventState &state);

    S_Key key() const;
    void setKey(const S_Key &key);

    S_KeyboardEventState state() const;
    void setState(const S_KeyboardEventState &state);

private:
    S_Key m_key;
    S_KeyboardEventState m_state;
};

class S_CharacterEvent : public S_Event
{
public:
    S_CharacterEvent(const char *character );

    const char *character() const;
    void setCharacter(const char *character);

private:
    char m_character[4];
};

class S_MouseEvent : public S_Event
{
public:
    S_MouseEvent(const S_MouseButton& button = S_MouseButton::None,
                 const S_MouseEventState &state = S_MouseEventState::Up,
                 unsigned int x = 0, unsigned int y = 0, int z = 0);

    S_MouseButton button() const;
    void setButton(const S_MouseButton &button);

    S_MouseEventState state() const;
    void setState(const S_MouseEventState &state);

    unsigned int x() const;
    void setX(unsigned int x);

    unsigned int y() const;
    void setY(unsigned int y);

    int z() const;
    void setZ(int z);

private:
    S_MouseButton       m_button;
    S_MouseEventState   m_state;
    unsigned int        m_x;
    unsigned int        m_y;
    int                 m_z;
};

class S_TouchPoint
{
public:
    S_TouchPoint( const S_TouchEventState &state = S_TouchEventState::Up, float x = 0.0f, float y = 0.0f, int id = 0 );

    float x() const;
    void setX(float x);

    float y() const;
    void setY(float y);

    int id() const;
    void setId(int id);

    S_TouchEventState state() const;
    void setState(const S_TouchEventState &state);
    void operator=(const S_TouchPoint &other);
private:
    S_TouchEventState m_state;
    float m_x;
    float m_y;
    int m_id;

};

class S_TouchEvent : public S_Event
{
public:
    S_TouchEvent();
    unsigned int activeCount() const;
    void setActiveCount(unsigned int activeCount);
    const S_TouchPoint &pointByIndex( unsigned int index );
    const S_TouchPoint &pointByID( int id );
    void setPointByIndex( unsigned int index, const S_TouchPoint &point );
    void setPointByID( int id, const S_TouchPoint &point );
    void operator=( S_TouchEvent &other );
private:
    static const char m_MAX_TOUCH_POINTS = 10;
    S_TouchPoint m_touchPoints[m_MAX_TOUCH_POINTS];
    unsigned int m_activeCount;
};



}
