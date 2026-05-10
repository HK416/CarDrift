#include "stdafx.h"
#include "ShaderLayout.h"
#include "Renderer.h"

ShaderLayout::ShaderLayout(RenderContext* context) : m_context(context) {}

ShaderLayout::~ShaderLayout() {
    VkDevice device = m_context->getDevice();

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
    }

    for (auto layout : m_descriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }
}

ShaderLayoutBuilder& ShaderLayoutBuilder::addDescriptorSetLayout(
    const std::vector<VkDescriptorSetLayoutBinding>& bindings
) {
    m_descriptorSets.push_back({bindings});
    return *this;
}

ShaderLayoutBuilder& ShaderLayoutBuilder::addPushConstantRange(
    VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size
) {
    VkPushConstantRange range = {};
    range.stageFlags = stageFlags;
    range.offset = offset;
    range.size = size;
    m_pushConstantRanges.push_back(range);
    return *this;
}

std::unique_ptr<ShaderLayout> ShaderLayoutBuilder::build(RenderContext* context) {
    auto shaderLayout = std::make_unique<ShaderLayout>(context);
    VkDevice device = context->getDevice();

    // 1. Create Descriptor Set Layouts
    for (const auto& setInfo : m_descriptorSets) {
        VkDescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = static_cast<uint32_t>(setInfo.bindings.size());
        createInfo.pBindings = setInfo.bindings.data();

        VkDescriptorSetLayout layout;
        if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }

        shaderLayout->m_descriptorSetLayouts.push_back(layout);
    }

    // 2. Create Pipeline Layout
    VkPipelineLayoutCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = static_cast<uint32_t>(shaderLayout->m_descriptorSetLayouts.size());
    createInfo.pSetLayouts = shaderLayout->m_descriptorSetLayouts.data();
    createInfo.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
    createInfo.pPushConstantRanges = m_pushConstantRanges.data();

    if (vkCreatePipelineLayout(device, &createInfo, nullptr, &shaderLayout->m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    return shaderLayout;
}
