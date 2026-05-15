#pragma once
#include "GameScene.h"
#include "GameObject.h"

class Mesh;
class Material;
class Shader;
class ShaderLayout;
class Texture;

template<typename T>
using Cache = std::unordered_map<std::string, T>;

class TestbedScene : public GameScene {
public:
    TestbedScene() = delete;
    TestbedScene(const TestbedScene&) = delete;
    TestbedScene& operator=(const TestbedScene&) = delete;
    TestbedScene(Renderer* renderer, SceneManager* manager);
    virtual ~TestbedScene() = default;

    virtual void onEnter() override;
    virtual void update(float elapsedTimeSec) override;
    virtual void render(RenderQueue& queue, float alpha) override;

private:
    void createCubeMesh(VkCommandBuffer cmd);

protected:
    Cache<std::unique_ptr<ShaderLayout>> m_shaderLayouts;
    Cache<std::unique_ptr<Shader>> m_shaders;
    Cache<std::unique_ptr<Mesh>> m_meshes;
    Cache<std::unique_ptr<Material>> m_materials;
    Cache<std::unique_ptr<Texture>> m_textures;

    std::vector<std::unique_ptr<GameObject>> m_allObjects;
    std::vector<GameObject*> m_rootObjects;
};
