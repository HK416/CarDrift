#include "stdafx.h"
#include "Mesh.h"
#include "Renderer.h"

Mesh::Mesh(RenderContext* context) : m_context(context) {}

Mesh::~Mesh() {
    cleanupStaging();

    VmaAllocator allocator = m_context->getAllocator();
    for (auto& [attr, res] : m_vertexBuffers) {
        vmaDestroyBuffer(allocator, res.buffer, res.allocation);
    }

    if (m_indexBuffer.buffer) {
        vmaDestroyBuffer(allocator, m_indexBuffer.buffer, m_indexBuffer.allocation);
    }
}

void Mesh::bindBuffers(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout) {
    if (m_indexBuffer.buffer == VK_NULL_HANDLE ||
        m_geometrySet == VK_NULL_HANDLE) {
        return;
    }

    // Bind Index Buffer
    vkCmdBindIndexBuffer(cmd, m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Bind Descriptor Set
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        1, &m_geometrySet,
        0, nullptr
    );
}

void Mesh::cleanupStaging() {
    VmaAllocator allocator = m_context->getAllocator();
    for (auto& s : m_stagingResources) {
        vmaDestroyBuffer(allocator, s.buffer, s.allocation);
    }
    m_stagingResources.clear();
}

MeshBuilder& MeshBuilder::setPosition(const std::vector<glm::vec3>& pos) {
    m_position = pos;
    return *this;
}

MeshBuilder& MeshBuilder::setNormals(const std::vector<glm::vec3>& norm) {
    m_normals = norm;
    return *this;
}

MeshBuilder& MeshBuilder::setTangents(const std::vector<glm::vec4>& tan) {
    m_tangents = tan;
    return *this;
}

MeshBuilder& MeshBuilder::setTexcoord0(const std::vector<glm::vec2>& uvs) {
    m_texcoords0 = uvs;
    return *this;
}

MeshBuilder& MeshBuilder::setTexcoord1(const std::vector<glm::vec2>& uvs) {
    m_texcoords1 = uvs;
    return *this;
}

MeshBuilder& MeshBuilder::setColors(const std::vector<glm::vec4>& colors) {
    m_colors = colors;
    return *this;
}

MeshBuilder& MeshBuilder::setJointIndices(const std::vector<glm::ivec4>& indices) {
    m_jointIndices = indices;
    return *this;
}

MeshBuilder& MeshBuilder::setJointWeights(const std::vector<glm::vec4>& weights) {
    m_jointWeights = weights;
    return *this;
}

MeshBuilder& MeshBuilder::setIndices(const std::vector<uint32_t>& indices) {
    m_indices = indices;
    return *this;
}

MeshBuilder& MeshBuilder::addSubMesh(uint32_t start, uint32_t count) {
    m_subMeshes.push_back({start, count});
    return *this;
}

std::unique_ptr<Mesh> MeshBuilder::build(
    RenderContext* context, 
    VkCommandBuffer cmd, 
    VkDescriptorSetLayout layout
) {
    auto mesh = std::make_unique<Mesh>(context);

    // Attribute Position
    if (!m_position.empty()) {
        upload(
            context,
            cmd,
            mesh.get(),
            VertexAttribute::Position,
            m_position.data(),
            m_position.size() * sizeof(glm::vec3)
        );
    }

    // Attribute Normal
    if (!m_normals.empty()) {
        upload(
            context,
            cmd,
            mesh.get(),
            VertexAttribute::Normal,
            m_normals.data(),
            m_normals.size() * sizeof(glm::vec3)
        );
    }

    // Attribute Tangent
    if (!m_tangents.empty()) {
        upload(
            context,
            cmd,
            mesh.get(),
            VertexAttribute::Tangent,
            m_tangents.data(),
            m_tangents.size() * sizeof(glm::vec4)
        );
    }

    // Attribute Texcoords0
    if (!m_texcoords0.empty()) {
        upload(
            context,
            cmd,
            mesh.get(),
            VertexAttribute::Uv0,
            m_texcoords0.data(),
            m_texcoords0.size() * sizeof(glm::vec2)
        );
    }

    // Attribute Texcoords1
    if (!m_texcoords1.empty()) {
        upload(
            context,
            cmd,
            mesh.get(),
            VertexAttribute::Uv1,
            m_texcoords1.data(),
            m_texcoords1.size() * sizeof(glm::vec2)
        );
    }

    // Attribute Color
    if (!m_colors.empty()) {
        upload(
            context,
            cmd,
            mesh.get(),
            VertexAttribute::Color,
            m_colors.data(),
            m_colors.size() * sizeof(glm::vec4)
        );
    }

    // Attribute Joint Index
    if (!m_jointIndices.empty()) {
        upload(
            context,
            cmd,
            mesh.get(),
            VertexAttribute::JointIndex,
            m_jointIndices.data(),
            m_jointIndices.size() * sizeof(glm::ivec4)
        );
    }

    // Attribute Joint Weight
    if (!m_jointWeights.empty()) {
        upload(
            context,
            cmd,
            mesh.get(),
            VertexAttribute::JointWeight,
            m_jointWeights.data(),
            m_jointWeights.size() * sizeof(glm::vec4)
        );
    }

    // Index Buffer
    if (!m_indices.empty()) {
        uploadIndex(
            context,
            cmd,
            mesh.get(),
            m_indices.data(),
            m_indices.size() * sizeof(uint32_t)
        );
    }

    mesh->m_subMeshes = m_subMeshes;
    if (mesh->m_subMeshes.empty() && !m_indices.empty()) {
        mesh->m_subMeshes.push_back({0, static_cast<uint32_t>(m_indices.size())});
    }

    // Geometry Descriptor Set
    mesh->m_geometrySet = context->allocateDescriptorSet(layout);

    std::vector<VkDescriptorBufferInfo> bufferInfos(static_cast<uint32_t>(VertexAttribute::Count));
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(static_cast<uint32_t>(VertexAttribute::Count));

    for (uint32_t i = 0; i < static_cast<uint32_t>(VertexAttribute::Count); ++i) {
        VertexAttribute attr = static_cast<VertexAttribute>(i);

        auto it = mesh->m_vertexBuffers.find(attr);
        if (it != mesh->m_vertexBuffers.end()) {
            bufferInfos[i].buffer = it->second.buffer;
            bufferInfos[i].offset = 0;
            bufferInfos[i].range = VK_WHOLE_SIZE;

            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = mesh->m_geometrySet;
            write.dstBinding = i;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &bufferInfos[i];

            writes.emplace_back(write);
        }
    }

    if (!writes.empty()) {
        vkUpdateDescriptorSets(
            context->getDevice(),
            static_cast<uint32_t>(writes.size()),
            writes.data(),
            0,
            nullptr
        );
    }

    return mesh;
}

void MeshBuilder::upload(
    RenderContext* ctx,
    VkCommandBuffer cmd,
    Mesh* mesh,
    VertexAttribute attr,
    const void* data,
    VkDeviceSize size
) {
    VmaAllocator allocator = ctx->getAllocator();

    // Staging Buffer
    Mesh::BufferResource staging;
    VkBufferCreateInfo sCreateInfo = {};
    sCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    sCreateInfo.size = size;
    sCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo sAllocInfo = {};
    sAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    sAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo stagingAllocInfo;
    vmaCreateBuffer(
        allocator,
        &sCreateInfo,
        &sAllocInfo,
        &staging.buffer,
        &staging.allocation,
        &stagingAllocInfo
    );

    memcpy(stagingAllocInfo.pMappedData, data, size);
    mesh->m_stagingResources.push_back(staging);

    // GPU Only
    Mesh::BufferResource device;
    VkBufferCreateInfo dCreateInfo = {};
    dCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    dCreateInfo.size = size;
    dCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    VmaAllocationCreateInfo dAllocInfo = {};
    dAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    dAllocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    vmaCreateBuffer(
        allocator,
        &dCreateInfo,
        &dAllocInfo,
        &device.buffer,
        &device.allocation,
        nullptr
    );

    // Copy
    VkBufferCopy region{0, 0, size};
    vkCmdCopyBuffer(cmd, staging.buffer, device.buffer, 1, &region);

    mesh->m_vertexBuffers[attr] = device;
    mesh->m_attributeMask |= (1 << (uint32_t)attr);
}

void MeshBuilder::uploadIndex(
    RenderContext* ctx,
    VkCommandBuffer cmd,
    Mesh* mesh,
    const void* data,
    VkDeviceSize size
) {
    VmaAllocator allocator = ctx->getAllocator();

    // Staging Buffer
    Mesh::BufferResource staging;
    VkBufferCreateInfo sCreateInfo = {};
    sCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    sCreateInfo.size = size;
    sCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo sAllocInfo = {};
    sAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    sAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo stagingAllocInfo;
    vmaCreateBuffer(
        allocator,
        &sCreateInfo,
        &sAllocInfo,
        &staging.buffer,
        &staging.allocation,
        &stagingAllocInfo
    );

    memcpy(stagingAllocInfo.pMappedData, data, size);
    mesh->m_stagingResources.push_back(staging);

    // GPU Only
    Mesh::BufferResource device;
    VkBufferCreateInfo dCreateInfo = {};
    dCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    dCreateInfo.size = size;
    dCreateInfo.usage =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VmaAllocationCreateInfo dAllocInfo = {};
    dAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    dAllocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    vmaCreateBuffer(
        allocator,
        &dCreateInfo,
        &dAllocInfo,
        &device.buffer,
        &device.allocation,
        nullptr
    );

    // Copy
    VkBufferCopy region{0, 0, size};
    vkCmdCopyBuffer(cmd, staging.buffer, device.buffer, 1, &region);

    mesh->m_indexBuffer = device;
}
