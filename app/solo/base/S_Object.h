#pragma once
#include "solo/stl/S_List.h"

namespace solo
{

class S_Object
{
public:
    S_Object( S_Object *parent = nullptr );
    virtual ~S_Object();
    S_Object *parent() const;
    void setParent(S_Object *parent);
    const S_List<S_Object *> &children() const;

private:
    S_Object *m_parent;
    S_List<S_Object*> m_children;

};
}
