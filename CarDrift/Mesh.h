#pragma once

class RenderContext;
class MeshBuilder;

enum class VertexAttribute : uint32_t {
    Position = 0,
    Normal = 1,
    Tangent = 2,
    Uv0 = 3,
    Uv1 = 4,
    Color = 5,
    JointIndex = 6,
    JointWeight = 7,
    BoneMatrix = 8,
    Count
};

struct SubMesh {
    uint32_t indexStart;
    uint32_t indexCount;
};

class Mesh {
    friend class MeshBuilder;

public:
    Mesh() = delete;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(RenderContext* context);
    ~Mesh();

    void bindBuffers(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
    void cleanupStaging();

    const std::vector<SubMesh>& getSubMeshes() const { return m_subMeshes; }
    uint32_t getAttributeMask() const { return m_attributeMask; }

    VkDescriptorSet getGeometryDescriptorSet() const { return m_geometrySet; }
    VkBuffer getIndexBuffer() const { return m_indexBuffer.buffer; }

private:
    struct BufferResource {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };

    RenderContext* m_context; // 소유하지 않는 클래스 맴버

    VkDescriptorSet m_geometrySet = VK_NULL_HANDLE;
    std::map<VertexAttribute, BufferResource> m_vertexBuffers;
    BufferResource m_indexBuffer;

    std::vector<BufferResource> m_stagingResources;
    std::vector<SubMesh> m_subMeshes;
    uint32_t m_attributeMask = 0;

};

class MeshBuilder {
public:
    MeshBuilder& setPosition(const std::vector<glm::vec3>& pos);
    MeshBuilder& setNormals(const std::vector<glm::vec3>& norm);
    MeshBuilder& setTangents(const std::vector<glm::vec4>& tan);
    MeshBuilder& setTexcoord0(const std::vector<glm::vec2>& uvs);
    MeshBuilder& setTexcoord1(const std::vector<glm::vec2>& uvs);
    MeshBuilder& setColors(const std::vector<glm::vec4>& colors);
    MeshBuilder& setJointIndices(const std::vector<glm::ivec4>& indices);
    MeshBuilder& setJointWeights(const std::vector<glm::vec4>& weights);
    MeshBuilder& setBoneMatrices(const std::vector<glm::mat4>& matrices);

    MeshBuilder& setIndices(const std::vector<uint32_t>& indices);
    MeshBuilder& addSubMesh(uint32_t start, uint32_t count);

    std::unique_ptr<Mesh> build(
        RenderContext* context,
        VkCommandBuffer cmd,
        VkDescriptorSetLayout layout
    );

private:
    void upload(
        RenderContext* ctx,
        VkCommandBuffer cmd,
        Mesh* mesh,
        VertexAttribute attr,
        const void* data,
        VkDeviceSize size
    );

    void uploadIndex(
        RenderContext* ctx,
        VkCommandBuffer cmd,
        Mesh* mesh,
        const void* data,
        VkDeviceSize size
    );

private:
    std::vector<glm::vec3> m_position;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec4> m_tangents;
    std::vector<glm::vec2> m_texcoords0;
    std::vector<glm::vec2> m_texcoords1;
    std::vector<glm::vec4> m_colors;
    std::vector<glm::ivec4> m_jointIndices;
    std::vector<glm::vec4> m_jointWeights;
    std::vector<glm::mat4> m_boneMatrices;

    std::vector<uint32_t> m_indices;
    std::vector<SubMesh> m_subMeshes;
};
