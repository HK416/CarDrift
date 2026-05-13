#pragma once
#include "GameScene.h"
#include "GameObject.h"

class TestbedScene : public GameScene {
public:
    TestbedScene() = delete;
    TestbedScene(const TestbedScene&) = delete;
    TestbedScene& operator=(const TestbedScene&) = delete;

protected:
    std::vector<std::unique_ptr<GameObject>> m_allObjects;
    std::vector<GameObject*> m_rootObjects;
};
