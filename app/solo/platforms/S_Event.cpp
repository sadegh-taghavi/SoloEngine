#include "S_Event.h"

using namespace solo;

S_Event::S_Event(const S_EventType &type)
{
    m_type = type;
}

S_Event::~S_Event()
{

}

S_EventType S_Event::type() const
{
    return m_type;
}

void S_Event::setType(const S_EventType &type)
{
    m_type = type;
}
