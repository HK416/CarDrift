#include "stdafx.h"
#include "TestbedScene.h"
#include "ShaderLayout.h"
#include "Shader.h"
#include "Mesh.h"
#include "Material.h"
#include "Renderer.h"
#include "Texture.h"

class MeshObject : public GameObject {
public:
    MeshObject() = delete;
    MeshObject(const MeshObject&) = delete;
    MeshObject& operator=(const MeshObject&) = delete;
    MeshObject(Mesh* mesh, Material* material)
        : m_mesh(mesh), m_material(material) {}
    virtual ~MeshObject() = default;

    virtual void render(RenderQueue& queue) override {
        RenderItem item;
        item.mesh = m_mesh;
        item.material = m_material;
        item.worldMatrix = m_worldMatrix;
        queue.addOpaque(item);
        GameObject::render(queue);
    }

private:
    Mesh* m_mesh;
    Material* m_material;
};

TestbedScene::TestbedScene(Renderer* renderer, SceneManager* manager) 
    : GameScene(renderer, manager) {}

void TestbedScene::onEnter() {
    RenderContext* context = m_renderer->getContext();
    CommandManager* commandMgr = m_renderer->getCommandManager();
    VkCommandBuffer cmd = commandMgr->beginSingleTimeCommands();

    ShaderLayoutBuilder builder;
    builder.addDescriptorSetLayout(context->getGlobalDescriptorSetLayout());
    builder.addDescriptorSetLayout({
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
    });
    builder.addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4));
    m_shaderLayouts["Standard"] = builder.build(context);
    m_shaders["Standard"] = std::make_unique<StandardOpaqueShader>(context, m_shaderLayouts["Standard"].get());


    createCubeMesh(cmd);
    
    m_materials["Standard"] = std::make_unique<StandardMaterial>(context, m_shaders["Standard"].get());
    static_cast<StandardMaterial*>(m_materials["Standard"].get())->setAlbedo({1.0f, 0.5f, 0.3f, 1.0f});
    
    // 5x5 그리드로 큐브 배치
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            auto cube = std::make_unique<MeshObject>(m_meshes["Cube"].get(), m_materials["Standard"].get());
            cube->getTransform().setPosition({ (float)x * 2.0f, 0.0f, (float)z * 2.0f + 10.0f });
            
            m_rootObjects.push_back(cube.get());
            m_allObjects.push_back(std::move(cube));
        }
    }

    commandMgr->endSingleTimeCommands(cmd, context->getGraphicsQueue());
    vkQueueWaitIdle(context->getGraphicsQueue());
}

void TestbedScene::update(float elapsedTimeSec) {
    static float totalTime = 0.0f;
    totalTime += elapsedTimeSec;
    
    for (size_t i = 0; i < m_rootObjects.size(); ++i) {
        auto* obj = m_rootObjects[i];
        // 각 큐브마다 약간씩 다른 회전 애니메이션 적용
        obj->getTransform().setRotation(
            {totalTime * 50.0f + (float)i * 10.0f, totalTime * 30.0f, 0.0f}
        );
        obj->update(elapsedTimeSec);
    }
}

void TestbedScene::render(RenderQueue& queue, float alpha) {
    glm::mat4 view = glm::lookAt(
        glm::vec3({0.0f, 2.0f, -5.0f}),
        glm::vec3({0.0f, 0.0f, 0.5f}),
        glm::vec3({0.0f, 1.0f, 0.0f})
    );
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    proj[1][1] *= -1.0f;
    queue.setCamera(view, proj, glm::vec3(0.0f, 2.0f, -5.0f));

    Light dirLight;
    dirLight.lightType = (uint32_t)LightType::Directional;
    dirLight.intensity = 1.0f;
    dirLight.yDir = -1.0f;
    queue.addLight(dirLight);

    for (auto* obj : m_rootObjects) {
        obj->render(queue);
    }
}
void TestbedScene::createCubeMesh(VkCommandBuffer cmd) {
    // 1. Positions (24 vertices for 6 faces to have hard edges/unique normals
    // per face)
    std::vector<glm::vec3> positions = {
        // Front face
        {-0.5f, -0.5f, 0.5f},
        {0.5f, -0.5f, 0.5f},
        {0.5f, 0.5f, 0.5f},
        {-0.5f, 0.5f, 0.5f},
        // Back face
        {0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f, -0.5f},
        {-0.5f, 0.5f, -0.5f},
        {0.5f, 0.5f, -0.5f},
        // Top face
        {-0.5f, 0.5f, 0.5f},
        {0.5f, 0.5f, 0.5f},
        {0.5f, 0.5f, -0.5f},
        {-0.5f, 0.5f, -0.5f},
        // Bottom face
        {-0.5f, -0.5f, -0.5f},
        {0.5f, -0.5f, -0.5f},
        {0.5f, -0.5f, 0.5f},
        {-0.5f, -0.5f, 0.5f},
        // Right face
        {0.5f, -0.5f, 0.5f},
        {0.5f, -0.5f, -0.5f},
        {0.5f, 0.5f, -0.5f},
        {0.5f, 0.5f, 0.5f},
        // Left face
        {-0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f, 0.5f},
        {-0.5f, 0.5f, 0.5f},
        {-0.5f, 0.5f, -0.5f}
    };

    // 2. Normals
    std::vector<glm::vec3> normals = {
        // Front
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        // Back
        {0.0f, 0.0f, -1.0f},
        {0.0f, 0.0f, -1.0f},
        {0.0f, 0.0f, -1.0f},
        {0.0f, 0.0f, -1.0f},
        // Top
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        // Bottom
        {0.0f, -1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
        // Right
        {1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        // Left
        {-1.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f}
    };

    // 3. Texture Coordinates (UVs)
    std::vector<glm::vec2> uvs = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, // Front
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, // Back
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, // Top
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, // Bottom
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, // Right
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}  // Left
    };

    // 4. Tangents & Colors (StandardOpaqueShader 파이프라인에서 레이아웃 검사를
    // 통과하기 위한 더미 값)
    std::vector<glm::vec4> tangents(24, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    std::vector<glm::vec4> colors(24, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // 5. Indices (2 triangles per face)
    std::vector<uint32_t> indices = {
        0,  1,  2,  2,  3,  0,  // Front
        4,  5,  6,  6,  7,  4,  // Back
        8,  9,  10, 10, 11, 8,  // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };

    // 6. Build Mesh
    MeshBuilder builder;
    builder.setPosition(positions)
        .setNormals(normals)
        .setTexcoord0(uvs)
        .setTexcoord1(uvs)
        .setTangents(tangents)
        .setColors(colors)
        .setIndices(indices);

    m_meshes["Cube"] = builder.build(m_renderer->getContext(), cmd);
}