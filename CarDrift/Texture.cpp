#include "stdafx.h"
#include "Texture.h"
#include "Renderer.h"

Texture::Texture(RenderContext* context) : m_context(context) {}

Texture::~Texture() {
    cleanupStaging();
    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_context->getDevice(), m_sampler, nullptr);
    }

    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_context->getDevice(), m_imageView, nullptr);
    }

    if (m_image != VK_NULL_HANDLE) {
        vmaDestroyImage(m_context->getAllocator(), m_image, m_allocation);
    }
}

void Texture::cleanupStaging() {
    VmaAllocator allocator = m_context->getAllocator();
    for (auto& s : m_stagingResources) {
        vmaDestroyBuffer(allocator, s.buffer, s.allocation);
    }
    m_stagingResources.clear();
}

void Texture::upload(VkCommandBuffer cmd, const TextureResourceData& data) {
    m_format = data.format;
    VmaAllocator allocator = m_context->getAllocator();

    // 1. Create Staging Buffer
    BufferResource staging;
    VkBufferCreateInfo sCreateInfo = {};
    sCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    sCreateInfo.size = static_cast<uint32_t>(data.pixelData.size());
    sCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo sAllocInfo = {};
    sAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    sAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo stagingInfo;
    if (vmaCreateBuffer(allocator, &sCreateInfo, &sAllocInfo, &staging.buffer, &staging.allocation, &stagingInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image buffer resource!");
    }

    memcpy(stagingInfo.pMappedData, data.pixelData.data(), data.pixelData.size());
    m_stagingResources.push_back(staging);

    // 2. Create GPU Image
    VkImageCreateInfo imgCreateInfo = {};
    imgCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imgCreateInfo.extent.width = data.width;
    imgCreateInfo.extent.height = data.height;
    imgCreateInfo.extent.depth = 1;
    imgCreateInfo.mipLevels = data.mipLevels;
    imgCreateInfo.arrayLayers = data.layerCount;
    imgCreateInfo.format = data.format;
    imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                          VK_IMAGE_USAGE_SAMPLED_BIT;
    imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    if (data.layerCount == 6) {
        imgCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VmaAllocationCreateInfo imgAllocInfo = {};
    imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (vmaCreateImage(allocator, &imgCreateInfo, &imgAllocInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image resource!");
    }

    // 3. UNDEFINED -> TRANSFER_DST_OPTIMAL
    transitionImageLayout(
        cmd,
        m_image,
        m_format,
        0,
        data.mipLevels,
        0,
        data.layerCount,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    // 4. Copy
    std::vector<VkBufferImageCopy> regions;
    if (data.subresources.empty()) {
        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = data.layerCount;
        region.imageExtent.width = data.width;
        region.imageExtent.height = data.height;
        region.imageExtent.depth = 1;
        regions.push_back(region);
    } else {
        uint32_t subIndex = 0;
        for (uint32_t layer = 0; layer < data.layerCount; ++layer) {
            for (uint32_t mip = 0; mip < data.mipLevels; ++mip) {
                const auto& sub = data.subresources[subIndex];
                VkBufferImageCopy region = {};
                region.bufferOffset = sub.offset;
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = mip;
                region.imageSubresource.baseArrayLayer = layer;
                region.imageSubresource.layerCount = 1;
                region.imageExtent.width = sub.width;
                region.imageExtent.height = sub.height;
                region.imageExtent.depth = 1;

                regions.push_back(region);
                subIndex += 1;
            }
        }
    }

    vkCmdCopyBufferToImage(
        cmd,
        staging.buffer,
        m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(regions.size()),
        regions.data()
    );

    // 5. TRNASFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    transitionImageLayout(
        cmd,
        m_image,
        m_format,
        0,
        data.mipLevels,
        0,
        data.layerCount,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void Texture::transitionImageLayout(
    VkCommandBuffer cmd,
    VkImage image,
    VkFormat format,
    uint32_t baseMip,
    uint32_t mipCount,
    uint32_t baseLayer,
    uint32_t layerCount,
    VkImageLayout oldLayout,
    VkImageLayout newLayout
) {
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = baseMip;
    barrier.subresourceRange.levelCount = mipCount;
    barrier.subresourceRange.baseArrayLayer = baseLayer;
    barrier.subresourceRange.layerCount = layerCount;

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void Texture::createImageView(uint32_t layerCount, uint32_t mipLevels) {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = m_image;
    createInfo.viewType = (layerCount == 6) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = m_format;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.levelCount = mipLevels;
    createInfo.subresourceRange.layerCount = layerCount;

    if (vkCreateImageView(m_context->getDevice(), &createInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }
}

void Texture::createSampler(
    uint32_t mipLevels,
    VkFilter min,
    VkFilter mag,
    VkSamplerAddressMode u,
    VkSamplerAddressMode v
) {
    VkSamplerCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = mag;
    createInfo.minFilter = min;
    createInfo.addressModeU = u;
    createInfo.addressModeV = v;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = 16.0f;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.maxLod = mipLevels > 1 ? static_cast<float>(mipLevels) : 0;

    if (vkCreateSampler(m_context->getDevice(), &createInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image sampler!");
    }
}

CommonTextureBuilder& CommonTextureBuilder::setFile(
    const std::filesystem::path& filePath, bool srgb
) {
    m_info.filePath = filePath;
    m_info.isSRGB = srgb;
    return *this;
}

CommonTextureBuilder& CommonTextureBuilder::setFilter(VkFilter min, VkFilter mag) {
    m_info.minFilter = min;
    m_info.magFilter = mag;
    return *this;
}

CommonTextureBuilder& CommonTextureBuilder::setWrap(VkSamplerAddressMode u, VkSamplerAddressMode v) {
    m_info.wrapU = u;
    m_info.wrapV = v;
    return *this;
}

std::unique_ptr<Texture> CommonTextureBuilder::build(RenderContext* context, VkCommandBuffer cmd) {
    auto texture = std::make_unique<Texture>(context);
    TextureResourceData resData;

    int w, h, ch;
    stbi_uc* pixels = stbi_load(m_info.filePath.string().c_str(), &w, &h, &ch, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error(std::format("Failed to load texture (PATH:{})", m_info.filePath.string()));
    }
    resData.width = w;
    resData.height = h;
    resData.format = m_info.isSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    resData.pixelData.resize(w * h * 4);
    memcpy(
        static_cast<void*>(resData.pixelData.data()),
        static_cast<const void*>(pixels),
        resData.pixelData.size()
    );
    stbi_image_free(pixels);

    texture->upload(cmd, resData);
    texture->createImageView(resData.layerCount, resData.mipLevels);
    texture->createSampler(resData.mipLevels, m_info.minFilter, m_info.magFilter, m_info.wrapU, m_info.wrapV);

    return texture;
}
