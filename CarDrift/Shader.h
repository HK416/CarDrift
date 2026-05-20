#pragma once

class RenderContext;
class ShaderLayout;

struct PushConstantData {
    glm::mat4 worldMatrix{1.0f};
    uint32_t attributeMask = 0;
    uint32_t cascadeIndex = 0;
};

enum class ShaderFeature : uint32_t {
    None        = 0,
    Skinned     = 1 << 0,
    Transparent = 1 << 1,
    Cutoff      = 1 << 2
};

inline ShaderFeature operator|(ShaderFeature a, ShaderFeature b) {
    return static_cast<ShaderFeature>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool hasFeature(uint32_t key, ShaderFeature feature) {
    return (key & static_cast<uint32_t>(feature)) != 0;
}

struct RenderPipelineStates {
    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    VkPipelineColorBlendStateCreateInfo colorBlend = {};
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};

    RenderPipelineStates();
};

class Shader {
public:
    Shader() = delete;
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(RenderContext* context, ShaderLayout* layout, uint32_t shaderKey = 0);
    virtual ~Shader();

    virtual void bind(VkCommandBuffer cmd) = 0;

    VkPipeline getPipeline() const { return m_pipeline; }
    ShaderLayout* getLayout() const { return m_layout; }
    uint32_t getKey() const { return m_shaderKey; }

protected:
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readSPIRVFile(const std::filesystem::path& filePath);

protected:
    RenderContext* m_context; // 소유하지 않는 클래스 맴버
    ShaderLayout* m_layout;   // 소유하지 않는 클래스 맴버 (장면이 관리함)

    uint32_t m_shaderKey;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};

class GraphicsShader : public Shader {
public:
    GraphicsShader() = delete;
    GraphicsShader(const GraphicsShader&) = delete;
    GraphicsShader& operator=(const GraphicsShader&) = delete;
    GraphicsShader(RenderContext* context, ShaderLayout* layout, uint32_t shaderKey);
    virtual ~GraphicsShader() = default;

    virtual void bind(VkCommandBuffer cmd) override;

protected:
    void setupRenderPipeline(
        const RenderPipelineStates& states,
        std::vector<VkPipelineShaderStageCreateInfo>& stages
    );
};

class ComputeShader : public Shader {
public:
    ComputeShader() = delete;
    ComputeShader(const ComputeShader&) = delete;
    ComputeShader& operator=(const ComputeShader&) = delete;
    ComputeShader(RenderContext* context, ShaderLayout* layout);
    virtual ~ComputeShader() = default;

    virtual void bind(VkCommandBuffer cmd) override;
    virtual void dispatch(
        VkCommandBuffer cmd,
        uint32_t groupCountX,
        uint32_t groupCountY,
        uint32_t groupCountZ
    );
};

class ShadowShader : public Shader {
public:
    ShadowShader() = delete;
    ShadowShader(const ShadowShader&) = delete;
    ShadowShader& operator=(const ShadowShader&) = delete;
    ShadowShader(
        RenderContext* context, ShaderLayout* layout, uint32_t shaderKey
    );
    virtual ~ShadowShader() = default;

    virtual void bind(VkCommandBuffer cmd) override;

protected:
    void setupShadowPipeline(
        const RenderPipelineStates& states,
        std::vector<VkPipelineShaderStageCreateInfo>& stages
    );
};

class StandardShader : public GraphicsShader {
public:
    StandardShader() = delete;
    StandardShader(const StandardShader&) = delete;
    StandardShader& operator=(const StandardShader&) = delete;
    StandardShader(
        RenderContext* context, ShaderLayout* layout, uint32_t shaderKey
    );
    virtual ~StandardShader() = default;
};

class StandardShadowShader : public ShadowShader {
public:
    StandardShadowShader() = delete;
    StandardShadowShader(const StandardShadowShader&) = delete;
    StandardShadowShader& operator=(const StandardShadowShader&) = delete;
    StandardShadowShader(
        RenderContext* context, ShaderLayout* layout, uint32_t shaderKey
    );
    virtual ~StandardShadowShader() = default;
};
