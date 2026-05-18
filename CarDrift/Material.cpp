#include "stdafx.h"
#include "Material.h"
#include "Shader.h"
#include "ShaderLayout.h"
#include "Renderer.h"
#include "Texture.h"

Material::Material(RenderContext* context, Shader* shader, bool transparent) 
    : m_context(context), m_shader(shader), m_isTransparent(transparent) {
    auto layouts = m_shader->getLayout()->getDescriptorSetLayout();
    if (layouts.size() > 2) {
        m_descriptorSet = m_context->allocateDescriptorSet(layouts[2]);
    }
}

void Material::bind(VkCommandBuffer cmd) {
    if (m_descriptorSet != VK_NULL_HANDLE) {
        VkPipelineLayout layout = m_shader->getLayout()->getPipelineLayout();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 2, 1, &m_descriptorSet, 0, nullptr);
    }
}

StandardMaterial::StandardMaterial(RenderContext* context, Shader* shader)
    : Material(context, shader), 
      m_albedoMap(context->getWhiteTextureSrgb()),
      m_normalMap(context->getFlatNormalTexture()),
      m_mrMap(context->getWhiteTextureUnorm()),
      m_aoMap(context->getWhiteTextureUnorm()) {
    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = sizeof(PBRMaterialParams);
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(m_context->getAllocator(), &createInfo, &allocInfo, &m_uniformBuffer, &m_allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create standard material uniform buffer!");
    }
}

StandardMaterial::~StandardMaterial() {
    if (m_uniformBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_context->getAllocator(), m_uniformBuffer, m_allocation);
    }
}

StandardMaterial& StandardMaterial::setAlbedo(const glm::vec4& color) {
    m_params.albedoFactor = color;
    m_dirty = true;
    return *this;
}

StandardMaterial& StandardMaterial::setMetallic(float m) {
    m_params.metallicFactor = m;
    m_dirty = true;
    return *this;
}

StandardMaterial& StandardMaterial::setRoughness(float r) {
    m_params.roughnessFactor = r;
    m_dirty = true;
    return *this;
}

StandardMaterial& StandardMaterial::setAlbedoMap(Texture* tex) {
    m_albedoMap = tex ? tex : m_context->getWhiteTextureSrgb();
    m_dirty = true;
    return *this;
}

StandardMaterial& StandardMaterial::setNormalMap(Texture* tex) {
    m_normalMap = tex ? tex : m_context->getFlatNormalTexture();
    m_dirty = true;
    return *this;
}

StandardMaterial& StandardMaterial::setMetallicRoughnessMap(Texture* tex) {
    m_mrMap = tex ? tex : m_context->getWhiteTextureUnorm();
    m_dirty = true;
    return *this;
}

StandardMaterial& StandardMaterial::setAOMap(Texture* tex) {
    m_aoMap = tex ? tex : m_context->getWhiteTextureUnorm();
    m_dirty = true;
    return *this;
}

void StandardMaterial::bind(VkCommandBuffer cmd) {
    if (m_dirty) {
        updateDescriptorSet();
        m_dirty = false;
    }

    Material::bind(cmd);
}

void StandardMaterial::updateDescriptorSet() {
    void* data;
    vmaMapMemory(m_context->getAllocator(), m_allocation, &data);
    memcpy(data, &m_params, sizeof(PBRMaterialParams));
    vmaUnmapMemory(m_context->getAllocator(), m_allocation);

    std::vector<VkWriteDescriptorSet> writes(5);

    // Binding 0: Uniform Buffer
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = m_uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(PBRMaterialParams);
    
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = m_descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo = &bufferInfo;
    
    // Binding 1: Albedo Map
    VkDescriptorImageInfo albedoImageInfo = {};
    albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    albedoImageInfo.imageView = m_albedoMap->getImageView();
    albedoImageInfo.sampler = m_albedoMap->getSampler();

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = m_descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &albedoImageInfo;
    
    // Binding 2: Normal Map
    VkDescriptorImageInfo normalImageInfo = {};
    normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normalImageInfo.imageView = m_normalMap->getImageView();
    normalImageInfo.sampler = m_normalMap->getSampler();

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = m_descriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;
    writes[2].pImageInfo = &normalImageInfo;

    // Binding 3: Metallic Roughness Map
    VkDescriptorImageInfo mrImageInfo = {};
    mrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    mrImageInfo.imageView = m_mrMap->getImageView();
    mrImageInfo.sampler = m_mrMap->getSampler();

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = m_descriptorSet;
    writes[3].dstBinding = 3;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[3].descriptorCount = 1;
    writes[3].pImageInfo = &mrImageInfo;

    // Binding 4: Ambient Occlusion Map
    VkDescriptorImageInfo aoImageInfo = {};
    aoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    aoImageInfo.imageView = m_aoMap->getImageView();
    aoImageInfo.sampler = m_aoMap->getSampler();

    writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[4].dstSet = m_descriptorSet;
    writes[4].dstBinding = 4;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[4].descriptorCount = 1;
    writes[4].pImageInfo = &aoImageInfo;

    vkUpdateDescriptorSets(
        m_context->getDevice(),
        static_cast<uint32_t>(writes.size()),
        writes.data(),
        0,
        nullptr
    );
}
