#pragma once
#include "RenderGraph.h"
#include "Transform.h"

class GameObject {
public:
    GameObject() = default;
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    virtual ~GameObject();

    Transform& getTransform() { return m_transform; }
    const glm::mat4& getWorldMatrix() const { return m_worldMatrix; }
    GameObject* getParent() const { return m_parent; }
    const std::vector<GameObject*>& getChildren() const { return m_children; }

    void update(float elapsedTimeSec);
    virtual void render(RenderQueue& queue);

    void setParent(GameObject* newParent);
    void removeParent();
    void addChild(GameObject* child);
    void removeChild(GameObject* child);

protected:
    void setWorldDirty();
    void updateWorldMatrix();

    virtual void onUpdate(float elapsedTimeSec) {}

protected:
    Transform m_transform;
    glm::mat4 m_worldMatrix{1.0f};

    GameObject* m_parent = nullptr; // 소유하지 않는 클래스 맴버
    std::vector<GameObject*> m_children; // 소유하지 않는 클래스 맴버

    bool m_worldDirty = true;
};
