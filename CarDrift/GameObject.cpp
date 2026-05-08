#include "stdafx.h"
#include "GameObject.h"

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
