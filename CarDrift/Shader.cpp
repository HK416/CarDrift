#include "stdafx.h"
#include "Shader.h"
#include "Renderer.h"
#include "ShaderLayout.h"

RenderPipelineStates::RenderPipelineStates() {
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.logicOpEnable = VK_FALSE;
    colorBlend.logicOp = VK_LOGIC_OP_COPY;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &colorBlendAttachment;
}

Shader::Shader(RenderContext* context, ShaderLayout* layout, uint32_t shaderKey)
    : m_context(context), m_layout(layout), m_shaderKey(shaderKey) {}

Shader::~Shader() {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_context->getDevice(), m_pipeline, nullptr);
    }
}

void Shader::bind(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

VkShaderModule Shader::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_context->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

std::vector<char> Shader::readSPIRVFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file " + filePath.string());
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

GraphicsShader::GraphicsShader(RenderContext* context, ShaderLayout* layout, uint32_t shaderKey) 
    : Shader(context, layout, shaderKey) {}

void GraphicsShader::bind(VkCommandBuffer cmd) {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    }
}

void GraphicsShader::setupRenderPipeline(
    const RenderPipelineStates& states,
    std::vector<VkPipelineShaderStageCreateInfo>& stages
) {
    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineRenderingCreateInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &RenderSwapchain::swapchainImageFormat;
    renderingInfo.depthAttachmentFormat = RenderSwapchain::depthImageFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
    pipelineInfo.pStages = stages.data();
    pipelineInfo.pVertexInputState = &vertexInputState;
    pipelineInfo.pInputAssemblyState = &states.inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &states.rasterizer;
    pipelineInfo.pMultisampleState = &states.multisampling;
    pipelineInfo.pDepthStencilState = &states.depthStencil;
    pipelineInfo.pColorBlendState = &states.colorBlend;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_layout->getPipelineLayout();

    if (vkCreateGraphicsPipelines(m_context->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
}

ComputeShader::ComputeShader(RenderContext* context, ShaderLayout* layout)
    : Shader(context, layout) {}

void ComputeShader::bind(VkCommandBuffer cmd) {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    }
}

void ComputeShader::dispatch(
    VkCommandBuffer cmd,
    uint32_t groupCountX,
    uint32_t groupCountY,
    uint32_t groupCountZ
) {
    vkCmdDispatch(cmd, groupCountX, groupCountY, groupCountZ);
}

StandardShader::StandardShader(RenderContext* context, ShaderLayout* layout, uint32_t shaderKey) 
    : GraphicsShader(context, layout, shaderKey) {
    // Load Shader Binary
    auto vertCode = readSPIRVFile("shaders/standard.vert.spv");
    auto fragCode = readSPIRVFile("shaders/standard.frag.spv");

    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    // Configuration Specialization Constants
    struct SpecializationData {
        VkBool32 hasSkinning;
        VkBool32 isTransparent;
        VkBool32 useCutoff;
    } specData;

    specData.hasSkinning = hasFeature(shaderKey, ShaderFeature::Skinned);
    specData.isTransparent = hasFeature(shaderKey, ShaderFeature::Transparent);
    specData.useCutoff = hasFeature(shaderKey, ShaderFeature::Cutoff);

    std::vector<VkSpecializationMapEntry> mapEntries = {
        {0, offsetof(SpecializationData, hasSkinning), sizeof(VkBool32)},
        {1, offsetof(SpecializationData, isTransparent), sizeof(VkBool32)},
        {2, offsetof(SpecializationData, useCutoff), sizeof(VkBool32)}
    };

    VkSpecializationInfo specInfo = {};
    specInfo.mapEntryCount = static_cast<uint32_t>(mapEntries.size());
    specInfo.pMapEntries = mapEntries.data();
    specInfo.dataSize = sizeof(SpecializationData);
    specInfo.pData = &specData;

    // Setup Shader Stages
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName = "main";
    shaderStages[0].pSpecializationInfo = &specInfo;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName = "main";
    shaderStages[1].pSpecializationInfo = &specInfo;

    // Configuration Dynamic States
    RenderPipelineStates states;

    if (specData.isTransparent) {
        states.depthStencil.depthWriteEnable = VK_FALSE;

        states.colorBlendAttachment.blendEnable = VK_TRUE;
        states.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        states.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        states.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        states.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        states.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        states.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    // Create Graphics Pipeline
    setupRenderPipeline(states, shaderStages);

    // Cleanup
    vkDestroyShaderModule(m_context->getDevice(), vertModule, nullptr);
    vkDestroyShaderModule(m_context->getDevice(), fragModule, nullptr);
}
