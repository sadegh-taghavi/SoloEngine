#include "S_VulkanTexture.h"
#include "solo/renderer/vulkan/S_VulkanRendererAPI.h"
#include "solo/resource/S_ResourceManager.h"
#include "solo/application/S_Application.h"
#include "solo/renderer/vulkan/S_VulkanAllocator.h"
#include "solo/file/S_File.h"
#include "solo/math/S_Math.h"
#include <vk_format.h>
#include <ktx.h>
#include <ktxint.h>
#include <gl_format.h>

using namespace solo;

S_VulkanTexture::S_VulkanTexture(S_VulkanRendererAPI *api, const S_String &texture) : S_Texture(), m_api( api )

{
    S_File file( texture );
    if( !file.open() )
    {
        s_debugLayer("Texture not found!", texture );
        return;
    }

    S_Vector<std::byte> fileData(file.size());
    file.read( reinterpret_cast<char *>( fileData.data() ), fileData.size() );
    file.close();

    ktxTexture* ktTexture;
    if( ktxTexture_CreateFromMemory( reinterpret_cast<uint8_t *>( fileData.data() ), fileData.size(),
                                     KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktTexture ) != KTX_SUCCESS )
    {
        s_debugLayer("Can't create texture!", texture );
        return;
    }

    uint32_t elementSize = ktxTexture_GetElementSize(ktTexture);
    bool canUseFasterPath;
    uint32_t numImageLayers = ktTexture->numLayers;
    VkImageCreateFlagBits createFlags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
    VkImageType imageType;
    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
    VkFormat vkFormat;
    VkImageUsageFlags usageFlags =  VK_IMAGE_USAGE_SAMPLED_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    uint32_t numImageLevels;
    VkFilter blitFilter;
    m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (ktTexture->isCubemap)
    {
        numImageLayers *= 6;
        createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    switch (ktTexture->numDimensions)
    {
    case 1:
        imageType = VK_IMAGE_TYPE_1D;
        viewType = ktTexture->isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        break;
    case 2:
        imageType = VK_IMAGE_TYPE_2D;
        if (ktTexture->isCubemap)
            viewType = ktTexture->isArray ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        else
            viewType = ktTexture->isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        break;
    case 3:
        imageType = VK_IMAGE_TYPE_3D;
        viewType = VK_IMAGE_VIEW_TYPE_3D;
        break;
    }

    vkFormat = vkGetFormatFromOpenGLInternalFormat(ktTexture->glInternalformat);
    if (vkFormat == VK_FORMAT_UNDEFINED)
        vkFormat = vkGetFormatFromOpenGLFormat(ktTexture->glFormat, ktTexture->glType);
    if (vkFormat == VK_FORMAT_UNDEFINED)
    {
        s_debugLayer("Can't create texture VK_FORMAT_UNDEFINED!" );
        return;
    }

    if (ktTexture->generateMipmaps)
        usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    VkImageFormatProperties fp;
    VkResult vResult;
    vResult = vkGetPhysicalDeviceImageFormatProperties(m_api->physicalDevice(), vkFormat, imageType, tiling, usageFlags, createFlags, &fp);
    if (vResult == VK_ERROR_FORMAT_NOT_SUPPORTED)
    {
        s_debugLayer("Can't create texture VK_ERROR_FORMAT_NOT_SUPPORTED!" );
        return;
    }

    if (ktTexture->generateMipmaps)
    {
        uint32_t max_dim;
        VkFormatProperties    formatProperties;
        VkFormatFeatureFlags  formatFeatureFlags;
        VkFormatFeatureFlags  neededFeatures = VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT;
        vkGetPhysicalDeviceFormatProperties( m_api->physicalDevice(), vkFormat, &formatProperties);

        if (tiling == VK_IMAGE_TILING_OPTIMAL)
            formatFeatureFlags = formatProperties.optimalTilingFeatures;
        else
            formatFeatureFlags = formatProperties.linearTilingFeatures;

        if ((formatFeatureFlags & neededFeatures) != neededFeatures)
        {
            s_debugLayer("Can't create texture Invalid operation!" );
            return;
        }

        if (formatFeatureFlags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
            blitFilter = VK_FILTER_LINEAR;
        else
            blitFilter = VK_FILTER_NEAREST; // XXX INVALID_OP?

        max_dim = solo::max(solo::max(ktTexture->baseWidth, ktTexture->baseHeight), ktTexture->baseDepth);
        numImageLevels = static_cast<uint32_t>(floor(log2(max_dim)) + 1);
    } else
    {
        numImageLevels = ktTexture->numLevels;
    }

    {
        uint32_t actualRowPitch = ktxTexture_GetRowPitch(ktTexture, 0);
        uint32_t tightRowPitch = elementSize * ktTexture->baseWidth;
        if (elementSize % 4 == 0  || (ktTexture->numLevels == 1 && actualRowPitch == tightRowPitch))
            canUseFasterPath = true;
        else
            canUseFasterPath = false;
    }

    uint32_t bufferSize = ktTexture->dataSize;
    uint32_t numCopyRegions;
    if (canUseFasterPath)
    {
        numCopyRegions = ktTexture->numLevels;
    } else
    {
        numCopyRegions = ktTexture->isArray ? ktTexture->numLevels : ktTexture->numLevels * ktTexture->numFaces;
        bufferSize += numCopyRegions * elementSize * 4;
    }
    S_Vector<VkBufferImageCopy> copyRegions(numCopyRegions);


    VkBuffer stageBuffer;
    S_VulkanDeviceMemory stageMemory;

    m_api->deviceAllocator()->createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                            stageBuffer, stageMemory );
    VkMappedMemoryRange vkrange = {};
    vkrange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    vkrange.size = stageMemory.ActualSize;
    vkrange.offset = stageMemory.OffsetFromBufferBase;
    vkrange.pNext = nullptr;
    vkrange.memory = stageMemory.Memory;

    if (canUseFasterPath)
    {
        memcpy( stageMemory.MappedPtr, ktTexture->pData, ktTexture->dataSize );
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t levelSize;
        uint64_t bufferOffset = 0;
        for (uint32_t miplevel = 0; miplevel < ktTexture->numLevels; ++miplevel)
        {
            width = solo::max(static_cast<uint32_t>(1), ktTexture->baseWidth  >> miplevel);
            height = solo::max(static_cast<uint32_t>(1), ktTexture->baseHeight >> miplevel);
            depth = solo::max(static_cast<uint32_t>(1), ktTexture->baseDepth  >> miplevel);

            levelSize = static_cast<uint32_t>( ktxTexture_levelSize(ktTexture, miplevel));
            //uint64_t offset;
            //ktxTexture_GetImageOffset(ktTexture, miplevel, 0, 0, &offset);

            VkBufferImageCopy *region = &copyRegions.at(miplevel);
            region->bufferOffset = bufferOffset;
            bufferOffset += levelSize;
            region->bufferRowLength = 0;
            region->bufferImageHeight = 0;
            region->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region->imageSubresource.mipLevel = miplevel;
            region->imageSubresource.baseArrayLayer = 0;
            region->imageSubresource.layerCount = ktTexture->numLayers * ktTexture->numFaces;
            region->imageOffset.x = 0;
            region->imageOffset.y = 0;
            region->imageOffset.z = 0;
            region->imageExtent.width = width;
            region->imageExtent.height = height;
            region->imageExtent.depth = depth;
        }

    }else
    {
        uint32_t faceLodSize;
        uint32_t face;
        uint32_t innerIterations;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint64_t bufferOffset = 0;
        uint32_t index = 0;
        uint8_t *pixelPtr;
        for (uint32_t miplevel = 0; miplevel < ktTexture->numLevels; ++miplevel)
        {
            width = solo::max(static_cast<uint32_t>(1), ktTexture->baseWidth  >> miplevel);
            height = solo::max(static_cast<uint32_t>(1), ktTexture->baseHeight >> miplevel);
            depth = solo::max(static_cast<uint32_t>(1), ktTexture->baseDepth  >> miplevel);

            faceLodSize = static_cast<uint32_t>(ktxTexture_faceLodSize(ktTexture, miplevel));

            if (ktTexture->isCubemap && !ktTexture->isArray)
                innerIterations = ktTexture->numFaces;
            else
                innerIterations = 1;
            for (face = 0; face < innerIterations; ++face)
            {
                uint64_t offset;

                ktxTexture_GetImageOffset(ktTexture, miplevel, 0, face, &offset);

                pixelPtr = ktTexture->pData + offset;

                uint32_t rowPitch = width * elementSize;

                VkBufferImageCopy *region = &copyRegions.at(index);

//                region->bufferOffset = stageMemory.OffsetFromBufferBase + bufferOffset;

                if (_KTX_PAD_UNPACK_ALIGN_LEN(rowPitch) == 0)
                {
                    memcpy( reinterpret_cast<void *>(reinterpret_cast<uint64_t>( stageMemory.MappedPtr ) + bufferOffset ),
                            pixelPtr, faceLodSize);
                    bufferOffset += faceLodSize;
                }else
                {
                    uint32_t imageIterations;
                    uint32_t rowPitch;
                    uint32_t paddedRowPitch;

                    if (ktTexture->numDimensions == 3)
                        imageIterations = depth;
                    else if (ktTexture->numLayers > 1)
                        imageIterations = ktTexture->numLayers * ktTexture->numFaces;
                    else
                        imageIterations = 1;
                    rowPitch = paddedRowPitch = width * elementSize;
                    paddedRowPitch = _KTX_PAD_UNPACK_ALIGN(paddedRowPitch);

                    for (uint32_t image = 0; image < imageIterations; image++)
                    {
                        for (uint32_t row = 0; row < height; row++)
                        {
                            memcpy( reinterpret_cast<void *>(reinterpret_cast<uint64_t>( stageMemory.MappedPtr ) + bufferOffset ),
                                    pixelPtr, rowPitch);
                            bufferOffset += rowPitch;
                            pixelPtr = pixelPtr + paddedRowPitch;
                        }
                    }
                }

                if ( bufferOffset % elementSize != 0 || bufferOffset % 4 != 0)
                {
                    uint32_t lcm = elementSize == 3 ? 12 : 4;
                    bufferOffset = (uint32_t)(lcm * ceil((float)bufferOffset / lcm));
                }
                region->bufferRowLength = 0;
                region->bufferImageHeight = 0;
                region->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region->imageSubresource.mipLevel = miplevel;
                region->imageSubresource.baseArrayLayer = face;
                region->imageSubresource.layerCount = ktTexture->numLayers * ktTexture->numFaces;
                region->imageOffset.x = 0;
                region->imageOffset.y = 0;
                region->imageOffset.z = 0;
                region->imageExtent.width = width;
                region->imageExtent.height = height;
                region->imageExtent.depth = depth;

                ++index;
            }
        }

    }

    VK_RESULT_CHECK( vkFlushMappedMemoryRanges( m_api->device(), 1, &vkrange ) );

    S_VulkanDeviceMemory memory;
    m_api->deviceAllocator()->createImage( ktTexture->baseWidth, ktTexture->baseHeight,
                                           ktTexture->baseDepth, numImageLevels,
                                           numImageLayers, vkFormat, imageType,
                                           VK_IMAGE_USAGE_SAMPLED_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                           m_image, memory );


    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = numImageLevels;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = numImageLayers;

    VkCommandBuffer cmdBuffer = m_api->beginSingleTimeTransferCommands();

    m_api->setImageLayout( cmdBuffer, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    vkCmdCopyBufferToImage( cmdBuffer, stageBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copyRegions.size(), copyRegions.data() );

    if (ktTexture->generateMipmaps)
    {
        VkImageSubresourceRange subresourceRange;
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = numImageLayers;

            m_api->setImageLayout( cmdBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
            for (uint32_t i = 1; i < numImageLevels; i++)
            {
                VkImageBlit imageBlit;
                memset(&imageBlit, 0, sizeof(imageBlit));

                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.layerCount = numImageLayers;
                imageBlit.srcSubresource.mipLevel = i-1;
                imageBlit.srcOffsets[1].x = solo::max(static_cast<uint32_t>(1), ktTexture->baseWidth  >> (i - 1));
                imageBlit.srcOffsets[1].y = solo::max(static_cast<uint32_t>(1), ktTexture->baseHeight >> (i - 1));
                imageBlit.srcOffsets[1].z = solo::max(static_cast<uint32_t>(1), ktTexture->baseDepth  >> (i - 1));

                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.layerCount = 1;
                imageBlit.dstSubresource.mipLevel = i;
                imageBlit.dstOffsets[1].x = solo::max(static_cast<uint32_t>(1), ktTexture->baseWidth  >> i);
                imageBlit.dstOffsets[1].y = solo::max(static_cast<uint32_t>(1), ktTexture->baseHeight >> i);
                imageBlit.dstOffsets[1].z = solo::max(static_cast<uint32_t>(1), ktTexture->baseDepth  >> i);

                VkImageSubresourceRange mipSubRange;
                memset(&mipSubRange, 0, sizeof(mipSubRange));

                mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                mipSubRange.baseMipLevel = i;
                mipSubRange.levelCount = 1;
                mipSubRange.layerCount = numImageLayers;

                m_api->setImageLayout( cmdBuffer, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipSubRange);

                vkCmdBlitImage( cmdBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_image,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, blitFilter);

                m_api->setImageLayout( cmdBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mipSubRange);
            }

            subresourceRange.levelCount = numImageLevels;
            m_api->setImageLayout( cmdBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_layout, subresourceRange);

    }else
    {
        m_api->setImageLayout( cmdBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_layout, subresourceRange);
    }
    m_api->endSingleTimeTransferCommands( cmdBuffer );

    ktxTexture_Destroy( ktTexture );

    m_api->deviceAllocator()->destroy( stageBuffer );

    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = m_image;
    createInfo.viewType = viewType;
    createInfo.format = vkFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = numImageLevels;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = numImageLayers;
    VK_RESULT_CHECK( vkCreateImageView( m_api->device(), &createInfo, S_VulkanAllocator(), &m_view) );
}

S_VulkanTexture::~S_VulkanTexture()
{
    vkDestroyImageView( m_api->device(), m_view, S_VulkanAllocator() );
    m_api->deviceAllocator()->destroy( m_image );
}

VkImage S_VulkanTexture::image() const
{
    return m_image;
}

VkImageView S_VulkanTexture::view() const
{
    return m_view;
}

VkImageLayout S_VulkanTexture::layout() const
{
    return m_layout;
}
