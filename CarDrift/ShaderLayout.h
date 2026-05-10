#pragma once

class RenderContext;
class ShaderLayoutBuilder;

class ShaderLayout {
    friend class ShaderLayoutBuilder;

public:
    ShaderLayout() = delete;
    ShaderLayout(const ShaderLayout&) = delete;
    ShaderLayout(RenderContext* context);
    ~ShaderLayout();

    VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
    const std::vector<VkDescriptorSetLayout>& getDescriptorSetLayout() const {
        return m_descriptorSetLayouts;
    }

private:
    RenderContext* m_context; // 소유하지 않는 클래스 맴버

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
};

class ShaderLayoutBuilder {
public: 
    ShaderLayoutBuilder& addDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
    ShaderLayoutBuilder& addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);

    std::unique_ptr<ShaderLayout> build(RenderContext* context);

private:
    struct DescriptorSetInfo {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };

    std::vector<DescriptorSetInfo> m_descriptorSets;
    std::vector<VkPushConstantRange> m_pushConstantRanges;
};
