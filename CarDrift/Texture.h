#pragma once

class RenderContext;
class CommonTextureBuilder;

struct SubresourceData {
    uint32_t offset;
    uint32_t size;
    uint32_t width;
    uint32_t height;
};

struct TextureResourceData {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    uint32_t layerCount = 1;
    VkFormat format = VK_FORMAT_UNDEFINED;
    std::vector<uint8_t> pixelData;
    std::vector<SubresourceData> subresources;
};

class Texture {
    friend class CommonTextureBuilder;

public:
    Texture() = delete;
    Texture(const Texture&) = delete;
    Texture(RenderContext* context);
    ~Texture();

    VkImageView getImageView() const { return m_imageView; }
    VkSampler getSampler() const { return m_sampler; }
    VkFormat getFormat() const { return m_format; }

    void cleanupStaging();
    void upload(VkCommandBuffer cmd, const TextureResourceData& data);

private:
    void transitionImageLayout(
        VkCommandBuffer cmd,
        VkImage image,
        VkFormat format,
        uint32_t baseMip,
        uint32_t mipCount,
        uint32_t baseLayer,
        uint32_t layerCount,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    );
    void createImageView(uint32_t layerCount, uint32_t mipLevels);
    void createSampler(uint32_t mipLevels, VkFilter min, VkFilter mag, VkSamplerAddressMode u, VkSamplerAddressMode v);

private:
    RenderContext* m_context; // 소유하지 않는 클래스 맴버

    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;

    struct BufferResource {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };

    std::vector<BufferResource> m_stagingResources;
};

class CommonTextureBuilder {
public:
    CommonTextureBuilder& setFile(const std::filesystem::path& filePath, bool srgb = true);
    CommonTextureBuilder& setFilter(VkFilter min, VkFilter mag);
    CommonTextureBuilder& setWrap(VkSamplerAddressMode u, VkSamplerAddressMode v);

    std::unique_ptr<Texture> build(RenderContext* context, VkCommandBuffer cmd);

private:
    struct BuildInfo {
        std::filesystem::path filePath;
        VkFilter minFilter = VK_FILTER_LINEAR;
        VkFilter magFilter = VK_FILTER_LINEAR;
        VkSamplerAddressMode wrapU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode wrapV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        bool isSRGB = true;
    } m_info;
};
