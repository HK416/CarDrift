#pragma once
#include "GameScene.h"
#include "GameObject.h"

class PerspectiveCamera;
class DirectionalLight;
class SkyboxObject;
class Renderer;

class TestbedScene : public GameScene {
public:
    TestbedScene() = delete;
    TestbedScene(const TestbedScene&) = delete;
    TestbedScene& operator=(const TestbedScene&) = delete;
    TestbedScene(Renderer* renderer, SceneManager* manager);
    virtual ~TestbedScene() = default;

    virtual void onEnter() override;
    virtual void render(RenderQueue& queue, float alpha) override;

    virtual void onGUI() override;

private:
    void createSkyboxCubeMesh(VkCommandBuffer cmd);

protected:
    PerspectiveCamera* m_mainCamera = nullptr;
    DirectionalLight* m_mainDirLight = nullptr;
    SkyboxObject* m_skybox = nullptr;
};
