#pragma once
#include "GameScene.h"
#include "RenderGraph.h"

class Texture;

class RenderContext {
public:
    RenderContext() = delete;
    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;
    RenderContext(GLFWwindow* window);
    ~RenderContext();

    VkInstance getInstance() const { return m_instance; }
    VkDevice getDevice() const { return m_device; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkSurfaceKHR getSurface() const { return m_surface; }
    uint32_t getGraphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VmaAllocator getAllocator() const { return m_allocator; }
    VkDescriptorPool getEngineDescriptorPool() const { return m_descriptorPool; }
    VkDescriptorPool getGuiDescriptorPool() const { return m_guiDescriptorPool; }

    VkDescriptorSetLayout getGeometryDescriptorSetLayout() const { return m_geometryLayout; }
    VkDescriptorSetLayout getGlobalDescriptorSetLayout() const { return m_globalLayout; }

    VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout);

    void createDefaultTextures(VkCommandBuffer cmd);
    Texture* getWhiteTextureSrgb() const { return m_whiteTextureSrgb.get(); }
    Texture* getWhiteTextureUnorm() const { return m_whiteTextureUnorm.get(); }
    Texture* getFlatNormalTexture() const { return m_normalTexture.get(); }

private:
    void createRenderInstance();
    void setupDebugMessenger();
    void createRenderSurface(GLFWwindow* window);
    void createRenderDevice();
    void createMemoryAllocator();
    void createDescriptorPools();
    void createDescriptorlLayouts();

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamilyIndex = 0;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;

    VmaAllocator m_allocator = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorPool m_guiDescriptorPool = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_geometryLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_globalLayout = VK_NULL_HANDLE;

#ifdef NDEBUG
    const bool m_enableValidationLayers = false;
#else
    const bool m_enableValidationLayers = true;
#endif
    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    std::unique_ptr<Texture> m_whiteTextureSrgb;
    std::unique_ptr<Texture> m_whiteTextureUnorm;
    std::unique_ptr<Texture> m_normalTexture;
};

class RenderSwapchain {
public:
    RenderSwapchain() = delete;
    RenderSwapchain(const RenderSwapchain&) = delete;
    RenderSwapchain& operator=(const RenderSwapchain&) = delete;
    RenderSwapchain(RenderContext* context, GLFWwindow* window);
    ~RenderSwapchain();

    VkSwapchainKHR getSwapchain() const { return m_swapchain; }
    VkExtent2D getExtent() const { return m_swapchainExtent; }
    const std::vector<VkImage>& getImages() const { return m_swapchainImages; }
    const std::vector<VkImageView>& getImageViews() const { return m_swapchainImageViews; }
    VkImageView getDepthImageView() const { return m_depthImageView; }
    VkImage getDepthImage() const { return m_depthImage; }

private:
    void createSwapchain(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, GLFWwindow* window);
    void createImageViews();
    void createDepthResources();

private:
    VkDevice m_device = VK_NULL_HANDLE; // 소유하지 않는 클래스 맴버
    VmaAllocator m_allocator = VK_NULL_HANDLE; // 소유하지 않는 클래스 맴버

private:
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkExtent2D m_swapchainExtent;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;

    VkImage m_depthImage = VK_NULL_HANDLE;
    VmaAllocation m_depthImageAllocation = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;

public:
    static const VkFormat swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    static const VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;
};

class CommandManager {
public:
    CommandManager() = delete;
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;
    CommandManager(RenderContext* context, size_t swapchainImageCount);
    ~CommandManager();

    VkCommandPool getCommandPool() const { return m_commandPool; }
    VkCommandBuffer getCommandBuffer(uint32_t index) const { return m_commandBuffers[index]; }

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue graphicsQueue);

private:
    void createCommandPool(uint32_t queueFamilyIndex);
    void createCommandBuffers(size_t swapchainImageCount);

private:
    VkDevice m_device = VK_NULL_HANDLE; // 소유하지 않는 클래스 맴버
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;
};

class Renderer {
public:
    Renderer() = delete;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(GLFWwindow* window);
    ~Renderer();

    void drawFrame(float elapsedTimeSec);
    void recreateSwapchain();
    void setFramebufferResized() { m_framebufferResized = true; }

    SceneManager* getSceneManager() const { return m_sceneManager.get(); }
    RenderContext* getContext() const { return m_context.get(); }
    CommandManager* getCommandManager() const { return m_commandManager.get(); }

    VkDescriptorSet getGlobalDescriptorSet(uint32_t frameIndex) const;
    void updateGlobalBuffer(uint32_t frameIndex, const GlobalData& data);

    VkImageView getShadowImageView() const { return m_shadowImageView; }
    VkSampler getShadowSampler() const { return m_shadowSampler; }
    
    RenderSwapchain* getSwapchain() const { return m_swapchain.get(); }
    VkImage getShadowImage() const { return m_shadowImage; }
    VkImageView getShadowLayerImageView(uint32_t layerIndex) const { return m_shadowLayerImageViews[layerIndex]; }
    static uint32_t getShadowMapSize() { return SHADOW_MAP_SIZE; }
    static uint32_t getShadowMapLayers() { return SHADOW_MAP_LAYERS; }

    void transitionImageLayout(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    );

private:
    void createSyncObjects();
    void createGlobalResources();
    void createShadowResources();

private:
    GLFWwindow* m_window = nullptr; // 소유하지 않는 클래스 맴버
    std::unique_ptr<RenderContext> m_context;
    std::unique_ptr<RenderSwapchain> m_swapchain;
    std::unique_ptr<CommandManager> m_commandManager;

    std::unique_ptr<SceneManager> m_sceneManager;

    VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence m_inFlightFence = VK_NULL_HANDLE;

    struct FrameResource {
        VkBuffer buffer;
        VmaAllocation allocation;
        VkDescriptorSet descriptorSet;
    };
    std::vector<FrameResource> m_globalResources;

    // Shadow Map Resources (Raw)
    VkImage m_shadowImage = VK_NULL_HANDLE;
    VmaAllocation m_shadowAllocation = VK_NULL_HANDLE;
    VkImageView m_shadowImageView = VK_NULL_HANDLE;
    VkSampler m_shadowSampler = VK_NULL_HANDLE;
    std::vector<VkImageView> m_shadowLayerImageViews;
    static const uint32_t SHADOW_MAP_SIZE = 2048;
    static const uint32_t SHADOW_MAP_LAYERS = 4;

    bool m_framebufferResized = false;
};
