#include "stdafx.h"
#include "GameObject.h"
#include "Material.h"

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

MeshObject::MeshObject(Mesh* mesh, Material* material) 
    : m_mesh(mesh), m_material(material) {}

void MeshObject::render(RenderQueue& queue) {
    if (!m_mesh || !m_material) {
        return;
    }

    RenderItem item;
    item.mesh = m_mesh;
    item.material = m_material;
    item.worldMatrix = m_worldMatrix;

    if (m_material->isTransparent()) {
        glm::vec3 camPos = queue.getGlobalData().cameraPos;
        item.sortDistance = glm::distance(camPos, getTransform().getPosition());
        queue.addTransparent(item);
    } else {
        queue.addOpaque(item);
    }
    
    GameObject::render(queue);
}
