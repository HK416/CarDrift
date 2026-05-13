#include "stdafx.h"
#include "Shader.h"
#include "Renderer.h"
#include "ShaderLayout.h"

VertexInputDescription VertexInputDescription::getStandardMeshLayout(bool skinned) {
    VertexInputDescription desc;

    if (skinned) {
        desc.bindings = {
            {0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX},  // Position
            {1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX},  // Normal
            {2, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX},  // Tangent
            {3, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX},  // UV0
            {4, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX},  // UV1
            {5, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX},  // Color
            {6, sizeof(glm::ivec4), VK_VERTEX_INPUT_RATE_VERTEX}, // Joint Index
            {7, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX}, // Joint Weight
        };

        desc.attributes = {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},    // Position
            {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},    // Normal
            {2, 2, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, // Tangent
            {3, 3, VK_FORMAT_R32G32_SFLOAT, 0},       // UV0
            {4, 4, VK_FORMAT_R32G32_SFLOAT, 0},       // UV1
            {5, 5, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, // Color
            {6, 6, VK_FORMAT_R32G32B32A32_SINT, 0},   // Joint Index
            {7, 7, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, // Joint Weight
        };
    } else {
        desc.bindings = {
            {0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // Position
            {1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // Normal
            {2, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX}, // Tangent
            {3, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}, // UV0
            {4, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}, // UV1
            {5, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX}, // Color
        };

        desc.attributes = {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},    // Position
            {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},    // Normal
            {2, 2, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, // Tangent
            {3, 3, VK_FORMAT_R32G32_SFLOAT, 0},       // UV0
            {4, 4, VK_FORMAT_R32G32_SFLOAT, 0},       // UV1
            {5, 5, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, // Color
        };
    }

    return desc;
}

RenderPipelineStates::RenderPipelineStates(bool skinned) {
    vertexInput = VertexInputDescription::getStandardMeshLayout(skinned);

    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &colorBlendAttachment;
}

Shader::Shader(RenderContext* context, ShaderLayout* layout)
    : m_context(context), m_layout(layout) {}

Shader::~Shader() {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_context->getDevice(), m_pipeline, nullptr);
    }
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

void Shader::setupRenderPipeline(
    const RenderPipelineStates& states,
    std::vector<VkPipelineShaderStageCreateInfo>& stages
) {
    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(states.vertexInput.bindings.size());
    vertexInputState.pVertexBindingDescriptions = states.vertexInput.bindings.data();
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(states.vertexInput.attributes.size());
    vertexInputState.pVertexAttributeDescriptions = states.vertexInput.attributes.data();

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

StandardOpaqueShader::StandardOpaqueShader(RenderContext* context, ShaderLayout* layout) 
    : Shader(context, layout) {
    auto vertCode = readSPIRVFile("shaders/standard.vert.spv");
    auto fragCode = readSPIRVFile("shaders/standard.frag.spv");

    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName = "main";

    RenderPipelineStates states;
    setupRenderPipeline(states, shaderStages);

    vkDestroyShaderModule(m_context->getDevice(), vertModule, nullptr);
    vkDestroyShaderModule(m_context->getDevice(), fragModule, nullptr);
}

void StandardOpaqueShader::bind(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}
