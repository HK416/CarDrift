#include "stdafx.h"
#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"

GameObject::~GameObject() {
    removeParent();

    for (auto child : m_children) {
        child->setParent(nullptr);
    }
}

void GameObject::update(float elapsedTimeSec) {
    // 1. 객체 고유의 로직 수행
    onUpdate(elapsedTimeSec);

    // 2. 월드 행렬 갱신
    if (m_transform.isDirty() || m_worldDirty) {
        updateWorldMatrix();
    }

    // 3. 전파
    for (auto child : m_children) {
        child->update(elapsedTimeSec);
    }
}

void GameObject::lateUpdate(float elapsedTimeSec) {
    // 1. 객체 고유의 로직 수행
    onLateUpdate(elapsedTimeSec);

    // 2. 전파
    for (auto child : m_children) {
        child->lateUpdate(elapsedTimeSec);
    }
}

void GameObject::render(RenderQueue& queue) {
    for (auto child : m_children) {
        child->render(queue);
    }
}

void GameObject::setParent(GameObject* newParent) {
    if (m_parent == newParent) {
        return;
    }

    // 1. 기존 부모의 자식 목록에서 자신을 제거
    if (m_parent) {
        auto& siblings = m_parent->m_children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    }

    // 2. 부모 교체
    m_parent = newParent;

    // 3. 새로운 부모의 자식 목록에 자신을 추가
    if (m_parent) {
        m_parent->m_children.push_back(this);
    }

    // 행렬 재계산 설정
    setWorldDirty();
}

void GameObject::removeParent() {
    setParent(nullptr);
}

void GameObject::addChild(GameObject* child) {
    if (child) {
        child->setParent(this);
    }
}

void GameObject::removeChild(GameObject* child) {
    if (child && child->m_parent == this) {
        child->setParent(nullptr);
    }
}

void GameObject::destroy() {
    if (m_isPendingDestroy) {
        return;
    }
    m_isPendingDestroy = true;

    if (m_parent && !m_parent->isPendingDestroy()) {
        removeParent();
    }
    m_parent = nullptr;

    for (GameObject* child : m_children) {
        if (child) {
            child->destroy();
        }
    }

    m_children.clear();
}

void GameObject::setWorldDirty() {
    m_worldDirty = true;
    for (auto child : m_children) {
        child->setWorldDirty();
    }
}

void GameObject::updateWorldMatrix() {
    if (m_parent) {
        m_worldMatrix = m_parent->m_worldMatrix * m_transform.getLocalMatrix();
    } else {
        m_worldMatrix = m_transform.getLocalMatrix();
    }
    m_worldDirty = false;
}

MeshObject::MeshObject(Mesh* mesh, const std::vector<Material*>& materials) 
    : m_mesh(mesh), m_materials(materials) {}

void MeshObject::render(RenderQueue& queue) {
    if (!m_mesh || m_materials.empty()) {
        return;
    }

    size_t subMeshCount = std::max<size_t>(1, m_mesh->getSubMeshes().size());

    for (size_t i = 0; i < subMeshCount; ++i) {
        Material* material = (i < m_materials.size()) ? m_materials[i] : m_materials[0];
        if (!material) {
            continue;
        }

        RenderItem item;
        item.mesh = m_mesh;
        item.material = material;
        item.worldMatrix = m_worldMatrix;
        item.submeshIndex = static_cast<uint32_t>(i);

        glm::vec3 camPos = queue.getGlobalData().cameraPos;
        item.sortDistance = glm::distance(camPos, getTransform().getPosition());

        if (material->isTransparent()) {
            queue.addTransparent(item);
        } else {
            queue.addOpaque(item);
        }
    }
    
    GameObject::render(queue);
}

SkinnedMeshObject::SkinnedMeshObject(Mesh* mesh, const std::vector<Material*>& materials) 
    : MeshObject(mesh, materials) {}

void SkinnedMeshObject::render(RenderQueue& queue) {
    if (!m_mesh || m_materials.empty()) {
        return;
    }

    size_t subMeshCount = std::max<size_t>(1, m_mesh->getSubMeshes().size());

    for (size_t i = 0; i < subMeshCount; ++i) {
        Material* material =
            (i < m_materials.size()) ? m_materials[i] : m_materials[0];
        if (!material) {
            continue;
        }

        RenderItem item;
        item.mesh = m_mesh;
        item.material = material;
        item.worldMatrix = m_worldMatrix;
        item.submeshIndex = static_cast<uint32_t>(i);
        item.boneMatrices = m_finalBoneMatrices.empty() ? nullptr : &m_finalBoneMatrices;

        glm::vec3 camPos = queue.getGlobalData().cameraPos;
        item.sortDistance = glm::distance(camPos, getTransform().getPosition());

        if (material->isTransparent()) {
            queue.addTransparent(item);
        } else {
            queue.addOpaque(item);
        }
    }

    GameObject::render(queue);
}

void SkinnedMeshObject::setBones(
    const std::vector<GameObject*>& bones,
    const std::vector<glm::mat4>& inverseBindMatrices
) {
    if (bones.size() != inverseBindMatrices.size()) {
        throw std::invalid_argument("TODO!");
    }

    m_bones = bones;
    m_inverseBindMatrices = inverseBindMatrices;
    m_finalBoneMatrices.resize(m_bones.size(), glm::mat4(1.0f));
}

void SkinnedMeshObject::onLateUpdate(float elapsedTimeSec) {
    MeshObject::onLateUpdate(elapsedTimeSec);

    if (!m_bones.empty()) {
        glm::mat4 invRootWorld = glm::inverse(getWorldMatrix());

        for (size_t i = 0; i < m_bones.size(); ++i) {
            if (m_bones[i]) {
                glm::mat4 boneWorldMat = m_bones[i]->getWorldMatrix();
                m_finalBoneMatrices[i] = invRootWorld * boneWorldMat * m_inverseBindMatrices[i];
            } else {
                m_finalBoneMatrices[i] = glm::mat4(1.0f);
            }
        }
    }
}

SkyboxObject::SkyboxObject(Mesh* cubeMesh, Material* skyboxMaterial)
    : MeshObject(cubeMesh, {skyboxMaterial}) {}
