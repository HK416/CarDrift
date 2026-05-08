#include "stdafx.h"
#include "Transform.h"

void Transform::setPosition(const glm::vec3& pos) {
    m_position = pos;
    m_dirty = true;
}

void Transform::setRotation(const glm::vec3& rot) {
    m_rotation = rot;
    m_dirty = true;
}

void Transform::setScale(const glm::vec3& scale) {
    m_scale = scale;
    m_dirty = true;
}

const glm::mat4& Transform::getLocalMatrix() const {
    if (m_dirty) {
        // Translate
        m_localMatrix = glm::translate(glm::mat4(1.0f), m_position);
        
        // Rotation (Y -> X -> Z)
        m_localMatrix = glm::rotate(m_localMatrix, glm::radians(m_rotation.y), {0.0f, 1.0f, 0.0f});
        m_localMatrix = glm::rotate(m_localMatrix, glm::radians(m_rotation.x), {1.0f, 0.0f, 0.0f});
        m_localMatrix = glm::rotate(m_localMatrix, glm::radians(m_rotation.z), {0.0f, 0.0f, 1.0f});

        // Scale
        m_localMatrix = glm::scale(m_localMatrix, m_scale);

        m_dirty = false;
    }

    return m_localMatrix;
}
