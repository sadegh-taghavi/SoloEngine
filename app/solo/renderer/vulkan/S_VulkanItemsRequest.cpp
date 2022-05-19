#include "S_VulkanRendererAPI.h"
#include "S_VulkanItemsRequest.h"
#include "S_VulkanAllocator.h"
#include "solo/debug/S_Debug.h"
#include "solo/stl/S_Map.h"
#include <stdint.h>
#include <algorithm>
#include <set>

using namespace solo;

const S_Vector<char *> *S_VulkanItemsRequest::enabledItems()
{
    return &m_enabledItems;
}

void S_VulkanItemsRequest::queryItems()
{
    m_enabledItems.clear();
    S_String active;
    for ( size_t i = 0; i < m_requestItems.size(); ++i )
    {
        active = m_requestItems.at( i );
        auto foundItem = std::find_if( m_items.begin(), m_items.end(),
                                       [&active](const char *item) { return S_String(item) == active;} );
        if ( foundItem != m_items.end() )
        {
            m_enabledItems.push_back( (*foundItem) );
//            s_debugLayer( "active item:", "\t", (*foundItem) );
        }
        else
            s_debugLayer( "item not found:", "\t", m_requestItems.at( i ) );
    }
}

S_VulkanItemsRequest::S_VulkanItemsRequest()
{

}

S_VulkanItemsRequest::~S_VulkanItemsRequest()
{

}

void S_VulkanItemsRequest::addRequestItem(const char* requestItem)
{
    m_requestItems.push_back( S_String( requestItem ) );
}

void solo::S_VulkanItemsRequest::clearRequestedItems()
{
    m_requestItems.clear();
}

void S_VulkanItemsInstanceExtensionRequest::queryItems()
{
    uint32_t count = 0;
    VK_RESULT_CHECK( vkEnumerateInstanceExtensionProperties( nullptr, &count, nullptr ) )
    S_Vector<VkExtensionProperties> items;
    items.resize( count );
    m_items.clear();
    VK_RESULT_CHECK( vkEnumerateInstanceExtensionProperties( nullptr, &count, items.data() ) )
    for ( size_t i = 0; i < items.size(); ++i )
    {
        m_items.push_back( items.at(i).extensionName  );
//        s_debugLayer( "S_VulkanItemsInstanceExtensionRequest:", "\t", items.at(i).extensionName );
    }
    S_VulkanItemsRequest::queryItems();
}

void S_VulkanItemsInstanceLayersRequest::queryItems()
{
    uint32_t count = 0;
    VK_RESULT_CHECK( vkEnumerateInstanceLayerProperties( &count, nullptr ) )
    S_Vector<VkLayerProperties> items;
    items.resize( count );
    m_items.clear();
    VK_RESULT_CHECK( vkEnumerateInstanceLayerProperties( &count, items.data() ) )
    for ( size_t i = 0; i < items.size(); ++i )
    {
        m_items.push_back( items.at(i).layerName  );
//        s_debugLayer( "S_VulkanItemsInstanceLayersRequest:", "\t", items.at(i).layerName );
    }
    S_VulkanItemsRequest::queryItems();
}

void S_VulkanItemsDeviceExtensionRequest::queryItems(S_VulkanRendererAPI *api)
{
    uint32_t count = 0;
    VK_RESULT_CHECK( vkEnumerateDeviceExtensionProperties( api->physicalDevice(), nullptr, &count, nullptr ) )
    S_Vector<VkExtensionProperties> items;
    items.resize( count );
    m_items.clear();
    VK_RESULT_CHECK( vkEnumerateDeviceExtensionProperties( api->physicalDevice(), nullptr, &count, items.data() ) )
    for ( size_t i = 0; i < items.size(); ++i )
    {
        m_items.push_back( items.at(i).extensionName  );
//        s_debugLayer( "S_VulkanItemsDeviceExtensionRequest:", "\t", items.at(i).extensionName );
    }
    S_VulkanItemsRequest::queryItems();
}

void S_VulkanItemsDeviceLayersRequest::queryItems(S_VulkanRendererAPI *api)
{
    uint32_t count = 0;
    VK_RESULT_CHECK( vkEnumerateDeviceLayerProperties( api->physicalDevice(), &count, nullptr ) )
    S_Vector<VkLayerProperties> items;
    items.resize( count );
    m_items.clear();
    VK_RESULT_CHECK( vkEnumerateDeviceLayerProperties( api->physicalDevice(), &count, items.data() ) )
    for ( size_t i = 0; i < items.size(); ++i )
    {
        m_items.push_back( items.at(i).layerName  );
//        s_debugLayer( "S_VulkanItemsDeviceLayersRequest:", "\t", items.at(i).layerName );
    }
    S_VulkanItemsRequest::queryItems();
}
