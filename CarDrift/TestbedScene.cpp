#include "stdafx.h"
#include "TestbedScene.h"
#include "ShaderLayout.h"
#include "Shader.h"
#include "Mesh.h"
#include "Material.h"
#include "Renderer.h"
#include "Texture.h"
#include "GameCamera.h"
#include "Light.h"

class CubeObject : public MeshObject {
public:
    CubeObject() = delete;
    CubeObject(const CubeObject&) = delete;
    CubeObject& operator=(const CubeObject&) = delete;
    CubeObject(Mesh* mesh, Material* material, uint32_t index)
        : MeshObject(mesh, material), m_index(index) {}
    virtual ~CubeObject() = default;

protected:
    virtual void onUpdate(float elapsedTimeSec) override {
        static float totalTime = 0.0f;
        totalTime += elapsedTimeSec;
        getTransform().setRotation(
            {totalTime * 5.0f + (float)m_index * 10.0f, totalTime * 3.0f, 0.0f}
        );
    }

protected:
    uint32_t m_index = 0;
};

TestbedScene::TestbedScene(Renderer* renderer, SceneManager* manager) 
    : GameScene(renderer, manager) {}

void TestbedScene::onEnter() {
    RenderContext* context = m_renderer->getContext();
    CommandManager* commandMgr = m_renderer->getCommandManager();
    VkCommandBuffer cmd = commandMgr->beginSingleTimeCommands();

    // --- Lights ---
    auto dirLight = std::make_unique<DirectionalLight>();
    dirLight->getTransform().setRotation({50.0f, 0.0f, -30.0f});
    dirLight->setColor({1.0f, 1.0f, 1.0f});
    dirLight->setIntensity(2.5f);
    m_rootObjects.push_back(dirLight.get());
    m_allObjects.push_back(std::move(dirLight));

    // --- Cameras ---
    auto camera = std::make_unique<PerspectiveCamera>();
    camera->getTransform().setPosition({0.0f, 2.0f, -5.0f});
    
    glm::mat4 lookAt = glm::lookAt(
        glm::vec3(0.0f, 2.0f, -5.0f),
        glm::vec3(0.0f, 0.0f, 0.5f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    camera->getTransform().setRotation(glm::quat_cast(glm::inverse(lookAt)));

    m_mainCamera = camera.get();
    m_rootObjects.push_back(camera.get());
    m_allObjects.push_back(std::move(camera));

    // --- Shader Layouts ---
    ShaderLayoutBuilder builder;
    builder.addDescriptorSetLayout(context->getGeometryDescriptorSetLayout());
    builder.addDescriptorSetLayout(context->getGlobalDescriptorSetLayout());
    builder.addDescriptorSetLayout({
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
    });
    builder.addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData));
    addShaderLayout("Standard", builder.build(context));


    // --- Shaders ---
    auto shader = std::make_unique<StandardShader>(context, getShaderLayout("Standard"), 0);
    addShader("Standard", std::move(shader));

    // --- Meshes ---
    createCubeMesh(cmd);
    
    // --- Materials ---
    auto material = std::make_unique<StandardMaterial>(context, getShader("Standard"));
    material->setAlbedo({1.0f, 0.5f, 0.3f, 1.0f});
    material->setMetallic(0.125f);
    material->setRoughness(0.8f);
    addMaterial("Standard", std::move(material));
    
    // --- Objects ---
    // 5x5 그리드로 큐브 배치
    uint32_t cnt = 0;
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            auto cube = std::make_unique<CubeObject>(getMesh("Cube"), getMaterial("Standard"), cnt++);
            cube->getTransform().setPosition({ (float)x * 2.0f, 0.0f, (float)z * 2.0f + 10.0f });
            
            m_rootObjects.push_back(cube.get());
            m_allObjects.push_back(std::move(cube));
        }
    }

    commandMgr->endSingleTimeCommands(cmd, context->getGraphicsQueue());
    vkQueueWaitIdle(context->getGraphicsQueue());
}

void TestbedScene::update(float elapsedTimeSec) {
    for (size_t i = 0; i < m_rootObjects.size(); ++i) {
        auto* obj = m_rootObjects[i];
        obj->update(elapsedTimeSec);
    }
}

void TestbedScene::render(RenderQueue& queue, float alpha) {
    if (m_mainCamera) {
        m_mainCamera->applyToQueue(queue);
    }

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
        .setIndices(indices);

    VkDescriptorSetLayout layout = m_renderer->getContext()->getGeometryDescriptorSetLayout();
    addMesh("Cube", builder.build(m_renderer->getContext(), cmd, layout));
}
