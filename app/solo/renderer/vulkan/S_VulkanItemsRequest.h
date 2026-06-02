#pragma once
#include "solo/renderer/S_RendererAPI.h"
#include <vector>
#include <string>
#include "solo/debug/S_Debug.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace solo
{

class S_VulkanRendererAPI;

class S_VulkanItemsRequest
{
public:
    S_VulkanItemsRequest();
    virtual ~S_VulkanItemsRequest();
    void addRequestItem(const char *requestItem);
    void clearRequestedItems();
    const std::vector<char *> *enabledItems();
protected:
    void queryItems();
    std::vector<std::string> m_items;
    std::vector<std::string> m_requestItems;
    std::vector<std::string> m_ownedEnabledItems;
    std::vector<char *> m_enabledItems;
};

class S_VulkanItemsInstanceExtensionRequest : public S_VulkanItemsRequest
{
public:
    void queryItems();

};

class S_VulkanItemsInstanceLayersRequest : public S_VulkanItemsRequest
{
public:
    void queryItems();

};

class S_VulkanItemsDeviceExtensionRequest : public S_VulkanItemsRequest
{
public:
    void queryItems(solo::S_VulkanRendererAPI *api );

};

class S_VulkanItemsDeviceLayersRequest : public S_VulkanItemsRequest
{
public:
    void queryItems(solo::S_VulkanRendererAPI *api );

};

}

