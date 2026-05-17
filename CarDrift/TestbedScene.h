#pragma once
#include "GameScene.h"
#include "GameObject.h"

class GameCamera;

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
    GameCamera* m_mainCamera = nullptr;

    std::vector<std::unique_ptr<GameObject>> m_allObjects;
    std::vector<GameObject*> m_rootObjects;
};
