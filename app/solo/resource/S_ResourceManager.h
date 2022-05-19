#pragma once

#include "solo/stl/S_String.h"
#include "S_Resources.h"
#include <thread>
#include <memory>


namespace solo
{

class S_ResourceData;

class S_ResourceManager
{

public:
    S_ResourceManager();
    const S_ResourceData *resourceData( const S_String &resourceName ) const;

private:
    std::unique_ptr<S_ResourcesContainer> m_container;

};

}
