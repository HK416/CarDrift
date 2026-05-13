#pragma once

class RenderContext;
class MeshBuilder;

enum class VertexAttribute : uint32_t {
    Position = 0,
    Normal = 1,
    Uv0 = 2,
    Color = 3,
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

    void bindBuffers(VkCommandBuffer cmd);
    void cleanupStaging();

    const std::vector<SubMesh>& getSubMeshes() const { return m_subMeshes; }
    uint32_t getAttributeMask() const { return m_attributeMask; }

private:
    struct BufferResource {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };

    RenderContext* m_context; // 소유하지 않는 클래스 맴버

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
    MeshBuilder& setTexcoord0(const std::vector<glm::vec2>& uvs);
    MeshBuilder& setColors(const std::vector<glm::vec4>& colors);

    MeshBuilder& setIndices(const std::vector<uint32_t>& indices);
    MeshBuilder& addSubMesh(uint32_t start, uint32_t count);

    std::unique_ptr<Mesh> build(RenderContext* context, VkCommandBuffer cmd);

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
    std::vector<glm::vec2> m_texcoords0;
    std::vector<glm::vec4> m_colors;

    std::vector<uint32_t> m_indices;
    std::vector<SubMesh> m_subMeshes;
};
