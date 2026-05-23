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
        for (const auto& sub : data.subresources) {
            VkBufferImageCopy region = {};
            region.bufferOffset = sub.offset;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = sub.mipLevel;
            region.imageSubresource.baseArrayLayer = sub.arrayLayer;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = sub.width;
            region.imageExtent.height = sub.height;
            region.imageExtent.depth = 1;
            regions.push_back(region);
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
    
    if (layerCount == 6) {
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    } else if (layerCount > 1) {
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    } else {
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    }

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
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.maxLod = mipLevels > 1 ? static_cast<float>(mipLevels) : 0;

    if (vkCreateSampler(m_context->getDevice(), &createInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image sampler!");
    }
}

PlainTextureBuilder& PlainTextureBuilder::setColor(uint32_t rawColor, bool srgb) {
    m_rawColor = rawColor;
    m_isSRGB = srgb;
    return *this;
}

std::unique_ptr<Texture> PlainTextureBuilder::build(RenderContext* context, VkCommandBuffer cmd) {
    auto texture = std::make_unique<Texture>(context);
    TextureResourceData resData;
    resData.width = 1;
    resData.height = 1;
    resData.format = m_isSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    resData.pixelData.resize(4);
    memcpy(resData.pixelData.data(), &m_rawColor, 4);

    texture->upload(cmd, resData);
    texture->createImageView(1, 1);
    texture->createSampler(
        1,
        VK_FILTER_NEAREST,
        VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT
    );

    return texture;
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

MemoryTextureBuilder& MemoryTextureBuilder::setEncodedData(
    const uint8_t* data, size_t size, bool srgb
) {
    m_info.bufferData.assign(data, data + size);
    m_info.isEncoded = true;
    m_info.isSRGB = srgb;
    m_info.mipLevels = 1;
    m_info.layerCount = 1;
    return *this;
}

MemoryTextureBuilder& MemoryTextureBuilder::setRawData(
    const uint8_t* pixels, uint32_t width, uint32_t height, VkFormat format
) {
    size_t dataSize = calculateFormatSize(width, height, format);

    m_info.bufferData.assign(pixels, pixels + dataSize);
    m_info.width = width;
    m_info.height = height;
    m_info.format = format;
    m_info.isEncoded = false;

    m_info.mipLevels = 1;
    m_info.layerCount = 1;

    m_info.subresources.clear();
    SubresourceData subData = {};
    subData.offset = 0;
    subData.size = static_cast<uint32_t>(dataSize);
    subData.width = width;
    subData.height = height;
    subData.mipLevel = 0;
    subData.arrayLayer = 0;
    m_info.subresources.push_back(subData);
    return *this;
}

MemoryTextureBuilder& MemoryTextureBuilder::setRawDataWithSubresources(
    const uint8_t* pixels,
    size_t totalDataSize,
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    uint32_t layerCount,
    VkFormat format,
    const std::vector<SubresourceData>& subresources
) {
    m_info.bufferData.assign(pixels, pixels + totalDataSize);
    m_info.width = width;
    m_info.height = height;
    m_info.mipLevels = mipLevels;
    m_info.layerCount = layerCount;
    m_info.format = format;
    m_info.subresources = subresources;
    m_info.isEncoded = false;
    return *this;
}

MemoryTextureBuilder& MemoryTextureBuilder::setFilter(VkFilter min, VkFilter mag) {
    m_info.minFilter = min;
    m_info.magFilter = mag;
    return *this;
}

MemoryTextureBuilder& MemoryTextureBuilder::setWrap(VkSamplerAddressMode u, VkSamplerAddressMode v) {
    m_info.wrapU = u;
    m_info.wrapV = v;
    return *this;
}

std::unique_ptr<Texture> MemoryTextureBuilder::build(RenderContext* context, VkCommandBuffer cmd) {
    TextureResourceData resData;

    if (m_info.isEncoded) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load_from_memory(
            m_info.bufferData.data(),
            static_cast<int>(m_info.bufferData.size()),
            &texWidth,
            &texHeight,
            &texChannels,
            STBI_rgb_alpha
        );

        if (!pixels) {
            throw std::runtime_error("Memory image decoding failed!");
        }

        resData.width = static_cast<uint32_t>(texWidth);
        resData.height = static_cast<uint32_t>(texHeight);
        resData.mipLevels = 1;
        resData.layerCount = 1;
        resData.format = m_info.isSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

        size_t imageSize = resData.width * resData.height * 4;
        resData.pixelData.assign(pixels, pixels + imageSize);

        SubresourceData subData = {};
        subData.offset = 0;
        subData.size = static_cast<uint32_t>(imageSize);
        subData.width = resData.width;
        subData.height = resData.height;
        subData.mipLevel = 0;
        subData.arrayLayer = 0;
        resData.subresources.push_back(subData);

        stbi_image_free(pixels);
    } else {
        if (m_info.bufferData.empty()) {
            throw std::runtime_error("Invalid Raw Data!");
        }

        resData.width = m_info.width;
        resData.height = m_info.height;
        resData.mipLevels = m_info.mipLevels;
        resData.layerCount = m_info.layerCount;
        resData.format = m_info.format;
        resData.pixelData = m_info.bufferData;
        resData.subresources = m_info.subresources;
    }

    auto texture = std::make_unique<Texture>(context);
    texture->upload(cmd, resData);
    texture->createImageView(resData.layerCount, resData.mipLevels);
    texture->createSampler(resData.mipLevels, m_info.minFilter, m_info.magFilter, m_info.wrapU, m_info.wrapV);

    return texture;
}

size_t MemoryTextureBuilder::calculateFormatSize(uint32_t width, uint32_t height, VkFormat format) const {
    switch (format) {
        case VK_FORMAT_R8_UNORM:            return width * height * 1;
        case VK_FORMAT_R8G8_UNORM:          return width * height * 2;
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SRGB:         return width * height * 3;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:       return width * height * 4;
        case VK_FORMAT_R16G16B16A16_SFLOAT: return width * height * 8;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return width * height * 16;

        // BC1, BC4 - 8byte per block
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK: {
            uint32_t blocksX = (width + 3) / 4;
            uint32_t blocksY = (height + 3) / 4;
            return blocksX * blocksY * 8;
        }
    
        // BC2, BC3, BC5, BC6H, BC7 - 16byte per block
        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK: {
            uint32_t blocksX = (width + 3) / 4;
            uint32_t blocksY = (height + 3) / 4;
            return blocksX * blocksY * 16;
        }

        default:
            throw std::runtime_error("Unknown Texture Format...");
    };
}
