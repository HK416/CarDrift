#pragma once
#include "GameScene.h"
#include "GameObject.h"

class TestbedScene : public GameScene {
protected:
    std::vector<std::unique_ptr<GameObject>> m_allObjects;
    std::vector<GameObject*> m_rootObjects;
};
