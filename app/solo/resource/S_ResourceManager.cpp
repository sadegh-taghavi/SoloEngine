#include "S_ResourceManager.h"
#include "solo/debug/S_Debug.h"

using namespace solo;

//std::unique_ptr<S_ResourcesContainer> S_ResourceManager::m_container;

S_ResourceManager::S_ResourceManager()
{
    if( !m_container.get() )
        m_container = std::make_unique<S_ResourcesContainer>();
}

const S_ResourceData *S_ResourceManager::resourceData(const S_String &resourceName) const
{
    auto it = m_container->resources()->find( resourceName );
    if( it == m_container->resources()->end() )
    {
        s_debugLayer( "Resource (", resourceName, ") not found!" );
        return nullptr;
    }else
        return it->second.get();
}
