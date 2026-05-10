#pragma once

class RenderContext;
class ShaderLayout;

struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    static VertexInputDescription getStandardMeshLayout(bool skinned);
};

struct RenderPipelineStates {
    RenderPipelineStates(bool skinned = false);

    VertexInputDescription vertexInput = {};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    VkPipelineColorBlendStateCreateInfo colorBlend = {};
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
};

class Shader {
public:
    Shader() = delete;
    Shader(const Shader&) = delete;
    Shader(RenderContext* context, std::shared_ptr<ShaderLayout> layout);
    virtual ~Shader();

    VkPipeline getPipeline() const { return m_pipeline; }
    std::shared_ptr<ShaderLayout> getLayout() const { return m_layout; }

    virtual void bind(VkCommandBuffer cmd) = 0;

protected:
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readSPIRVFile(const std::filesystem::path& filePath);
    void setupRenderPipeline(
        const RenderPipelineStates& states,
        std::vector<VkPipelineShaderStageCreateInfo>& stages
    );

protected:
    RenderContext* m_context; // 소유하지 않는 클래스 맴버

    std::shared_ptr<ShaderLayout> m_layout;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};

class StandardOpaqueShader : public Shader {
public:
    StandardOpaqueShader() = delete;
    StandardOpaqueShader(const StandardOpaqueShader&) = delete;
    StandardOpaqueShader(
        RenderContext* context, std::shared_ptr<ShaderLayout> layout
    );

    virtual void bind(VkCommandBuffer cmd) override;
};
