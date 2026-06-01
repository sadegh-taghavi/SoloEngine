#pragma once
#include <list>

namespace solo
{

class S_Object
{
public:
    S_Object( S_Object *parent = nullptr );
    virtual ~S_Object();
    S_Object *parent() const;
    void setParent(S_Object *parent);
    const std::list<S_Object *> &children() const;

private:
    S_Object *m_parent;
    std::list<S_Object*> m_children;

};
}
