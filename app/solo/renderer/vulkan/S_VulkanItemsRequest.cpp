#include "S_VulkanRendererAPI.h"
#include "S_VulkanItemsRequest.h"
#include "S_VulkanAllocator.h"
#include "solo/debug/S_Debug.h"
#include <map>
#include <stdint.h>
#include <algorithm>
#include <set>

using namespace solo;

const std::vector<char *> *S_VulkanItemsRequest::enabledItems()
{
    return &m_enabledItems;
}

void S_VulkanItemsRequest::queryItems()
{
    m_enabledItems.clear();
    m_ownedEnabledItems.clear();
    for ( size_t i = 0; i < m_requestItems.size(); ++i )
    {
        const std::string &active = m_requestItems.at( i );
        auto foundItem = std::find_if( m_items.begin(), m_items.end(),
                                       [&active](const std::string &item) { return item == active; } );
        if ( foundItem != m_items.end() )
            m_ownedEnabledItems.push_back( *foundItem );
        else
            s_debugLayer( "item not found:", "\t", active.c_str() );
    }
    for ( auto &s : m_ownedEnabledItems )
        m_enabledItems.push_back( const_cast<char *>( s.c_str() ) );
}

S_VulkanItemsRequest::S_VulkanItemsRequest()
{

}

S_VulkanItemsRequest::~S_VulkanItemsRequest()
{

}

void S_VulkanItemsRequest::addRequestItem(const char* requestItem)
{
    m_requestItems.push_back( std::string( requestItem ) );
}

void solo::S_VulkanItemsRequest::clearRequestedItems()
{
    m_requestItems.clear();
}

void S_VulkanItemsInstanceExtensionRequest::queryItems()
{
    uint32_t count = 0;
    VK_RESULT_CHECK( vkEnumerateInstanceExtensionProperties( nullptr, &count, nullptr ) )
    std::vector<VkExtensionProperties> items;
    items.resize( count );
    m_items.clear();
    VK_RESULT_CHECK( vkEnumerateInstanceExtensionProperties( nullptr, &count, items.data() ) )
    for ( size_t i = 0; i < items.size(); ++i )
    {
        m_items.push_back( std::string( items.at(i).extensionName ) );
//        s_debugLayer( "S_VulkanItemsInstanceExtensionRequest:", "\t", items.at(i).extensionName );
    }
    S_VulkanItemsRequest::queryItems();
}

void S_VulkanItemsInstanceLayersRequest::queryItems()
{
    uint32_t count = 0;
    VK_RESULT_CHECK( vkEnumerateInstanceLayerProperties( &count, nullptr ) )
    std::vector<VkLayerProperties> items;
    items.resize( count );
    m_items.clear();
    VK_RESULT_CHECK( vkEnumerateInstanceLayerProperties( &count, items.data() ) )
    for ( size_t i = 0; i < items.size(); ++i )
    {
        m_items.push_back( std::string( items.at(i).layerName ) );
//        s_debugLayer( "S_VulkanItemsInstanceLayersRequest:", "\t", items.at(i).layerName );
    }
    S_VulkanItemsRequest::queryItems();
}

void S_VulkanItemsDeviceExtensionRequest::queryItems(S_VulkanRendererAPI *api)
{
    uint32_t count = 0;
    VK_RESULT_CHECK( vkEnumerateDeviceExtensionProperties( api->physicalDevice(), nullptr, &count, nullptr ) )
    std::vector<VkExtensionProperties> items;
    items.resize( count );
    m_items.clear();
    VK_RESULT_CHECK( vkEnumerateDeviceExtensionProperties( api->physicalDevice(), nullptr, &count, items.data() ) )
    for ( size_t i = 0; i < items.size(); ++i )
    {
        m_items.push_back( std::string( items.at(i).extensionName ) );
//        s_debugLayer( "S_VulkanItemsDeviceExtensionRequest:", "\t", items.at(i).extensionName );
    }
    S_VulkanItemsRequest::queryItems();
}

void S_VulkanItemsDeviceLayersRequest::queryItems(S_VulkanRendererAPI *api)
{
    uint32_t count = 0;
    VK_RESULT_CHECK( vkEnumerateDeviceLayerProperties( api->physicalDevice(), &count, nullptr ) )
    std::vector<VkLayerProperties> items;
    items.resize( count );
    m_items.clear();
    VK_RESULT_CHECK( vkEnumerateDeviceLayerProperties( api->physicalDevice(), &count, items.data() ) )
    for ( size_t i = 0; i < items.size(); ++i )
    {
        m_items.push_back( std::string( items.at(i).layerName ) );
//        s_debugLayer( "S_VulkanItemsDeviceLayersRequest:", "\t", items.at(i).layerName );
    }
    S_VulkanItemsRequest::queryItems();
}
