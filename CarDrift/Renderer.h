#pragma once

class RenderContext {
public:
    RenderContext() = delete;
    RenderContext(const RenderContext&) = delete;
    RenderContext(GLFWwindow* window);
    ~RenderContext();

    VkInstance getInstance() const { return m_instance; }
    VkDevice getDevice() const { return m_device; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkSurfaceKHR getSurface() const { return m_surface; }
    uint32_t getGraphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VmaAllocator getAllocator() const { return m_allocator; }

private:
    void createRenderInstance();
    void setupDebugMessenger();
    void createRenderSurface(GLFWwindow* window);
    void createRenderDevice();
    void createMemoryAllocator();

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

#ifdef NDEBUG
    const bool m_enableValidationLayers = false;
#else
    const bool m_enableValidationLayers = true;
#endif
    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
};

class RenderSwapchain {
public:
    RenderSwapchain() = delete;
    RenderSwapchain(const RenderSwapchain&) = delete;
    RenderSwapchain(RenderContext* context, GLFWwindow* window);
    ~RenderSwapchain();

    VkSwapchainKHR getSwapchain() const { return m_swapchain; }
    VkFormat getFormat() const { return m_swapchainImageFormat; }
    VkExtent2D getExtent() const { return m_swapchainExtent; }
    const std::vector<VkImageView>& getImageViews() const { return m_swapchainImageViews; }
    VkImageView getDepthImageView() const { return m_depthImageView; }

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
    const VkFormat m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    VkImage m_depthImage = VK_NULL_HANDLE;
    VmaAllocation m_depthImageAllocation = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    const VkFormat m_depthImageFormat = VK_FORMAT_D32_SFLOAT;
};

//class CommandManager {
//public:
//    CommandManager(RenderContext* context);
//    ~CommandManager();
//
//    VkCommandPool getCommandPool() const { return m_commandPool; }
//    VkCommandBuffer getCommandBuffer(uint32_t index) const { return m_commandBuffers[index]; }
//
//private:
//    VkCommandPool m_commandPool = VK_NULL_HANDLE;
//    std::vector<VkCommandBuffer> m_commandBuffers;
//};
//
//class Renderer {
//public:
//    Renderer(GLFWwindow* window);
//    ~Renderer();
//
//    void drawFrame();
//
//private:
//    void createSyncObjects();
//
//private:
//    std::unique_ptr<RenderContext> m_context;
//    std::unique_ptr<ResourceManager> m_resourceManager;
//    std::unique_ptr<RenderSwapchain> m_swapchain;
//    std::unique_ptr<CommandManager> m_commandManager;
//
//    VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
//    VkSemaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;
//    VkFence m_inFlightFence = VK_NULL_HANDLE;
//
//    VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
//
//    bool m_framebufferResized = false;
//};
