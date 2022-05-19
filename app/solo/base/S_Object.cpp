#include "S_Object.h"
#include "solo/memory/S_Allocator.h"
using namespace solo;

S_Object::S_Object(S_Object *parent) : m_parent(parent)
{
    if( m_parent )
        m_parent->m_children.push_back( this );
}

S_Object::~S_Object()
{
    if( m_parent )
    {
        S_Object *obj = nullptr;
        while( !m_children.empty() )
        {
            obj = m_children.back();
            s_safe_delete(obj);
            m_children.pop_back();
        }
    }
}

S_Object *S_Object::parent() const
{
    return m_parent;
}

void S_Object::setParent(S_Object *parent)
{
    if( m_parent )
        m_parent->m_children.remove( this );
    m_parent = parent;
    if( m_parent )
        m_parent->m_children.push_back( this );
}

const S_List<S_Object *> &S_Object::children() const
{
    return m_children;
}
