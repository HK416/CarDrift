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
    void lateUpdate(float elapsedTimeSec);
    virtual void render(RenderQueue& queue);

    void setName(const std::string& name) { m_name = name; }
    const std::string& getName() const { return m_name; }

    void setParent(GameObject* newParent);
    void removeParent();
    void addChild(GameObject* child);
    void removeChild(GameObject* child);

    virtual void destroy(); 
    bool isPendingDestroy() const { return m_isPendingDestroy; }


protected:
    void setWorldDirty();
    virtual void updateWorldMatrix();
    virtual void onUpdate(float elapsedTimeSec) {}
    virtual void onLateUpdate(float elapsedTimeSec) {}

protected:
    std::string m_name = "GameObject";

    Transform m_transform;
    glm::mat4 m_worldMatrix{1.0f};

    GameObject* m_parent = nullptr; // 소유하지 않는 클래스 맴버
    std::vector<GameObject*> m_children; // 소유하지 않는 클래스 맴버

    bool m_worldDirty = true;
    bool m_isPendingDestroy = false;
};

class MeshObject : public GameObject {
public:
    MeshObject() = delete;
    MeshObject(const MeshObject&) = delete;
    MeshObject& operator=(const MeshObject&) = delete;
    MeshObject(Mesh* mesh, const std::vector<Material*>& materials);
    virtual ~MeshObject() = default;

    virtual void render(RenderQueue& queue) override;

    Mesh* getMesh() const { return m_mesh; }
    const std::vector<Material*>& getMaterials() const { return m_materials; }

protected:
    Mesh* m_mesh;
    std::vector<Material*> m_materials;
};

class SkinnedMeshObject : public MeshObject {
public:
    SkinnedMeshObject() = delete;
    SkinnedMeshObject(const SkinnedMeshObject&) = delete;
    SkinnedMeshObject& operator=(const SkinnedMeshObject&) = delete;
    SkinnedMeshObject(Mesh* mesh, const std::vector<Material*>& materials);
    virtual ~SkinnedMeshObject() = default;

    virtual void render(RenderQueue& queue) override;

    void setBones(const std::vector<GameObject*>& bones, const std::vector<glm::mat4>& inverseBindMatrices);
    const std::vector<glm::mat4>& getFinalBoneMatrices() const { return m_finalBoneMatrices; }

protected:
    virtual void onLateUpdate(float elapsedTimeSec) override;

protected:
    std::vector<GameObject*> m_bones;
    std::vector<glm::mat4> m_inverseBindMatrices;
    std::vector<glm::mat4> m_finalBoneMatrices;
};

class SkyboxObject : public MeshObject {
public:
    SkyboxObject() = delete;
    SkyboxObject(const SkyboxObject&) = delete;
    SkyboxObject& operator=(const SkyboxObject&) = delete;

    SkyboxObject(Mesh* cubeMesh, Material* skyboxMaterial);
    virtual ~SkyboxObject() = default;

    virtual void render(RenderQueue& queue) override {};
};
