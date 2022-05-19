#pragma once
#include "solo/renderer/S_RendererAPI.h"
#include "solo/stl/S_Vector.h"
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
    const S_Vector<char *> *enabledItems();
protected:
    void queryItems();
    S_Vector<char *> m_items;
    S_Vector<S_String> m_requestItems;
    S_Vector<char *> m_enabledItems;
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

