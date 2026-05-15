#include "stdafx.h"
#include "Transform.h"

void Transform::setPosition(const glm::vec3& pos) {
    m_position = pos;
    m_dirty = true;
}

void Transform::setRotation(const glm::quat& rot) {
    m_rotation = rot;
    m_dirty = true;
}

void Transform::setRotation(const glm::vec3& eulerDegrees) {
    m_rotation = glm::quat(glm::radians(eulerDegrees));
    m_dirty = true;
}

void Transform::setScale(const glm::vec3& scale) {
    m_scale = scale;
    m_dirty = true;
}

void Transform::rotate(const glm::quat& rot) {
    m_rotation = m_rotation * rot;
    m_rotation = glm::normalize(m_rotation);
    m_dirty = true;
}

void Transform::rotate(const glm::vec3& eulerDegrees) {
    rotate(glm::quat(glm::radians(eulerDegrees)));
}

glm::vec3 Transform::getEulerAngles() const {
    return glm::degrees(glm::eulerAngles(m_rotation));
}

const glm::mat4& Transform::getLocalMatrix() const {
    if (m_dirty) {
        // Translate
        glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), m_position);
        // Rotation
        glm::mat4 rotationMat = glm::mat4_cast(m_rotation);
        // Scale
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), m_scale);

        // TRS
        m_localMatrix = translationMat * rotationMat * scaleMat;

        m_dirty = false;
    }

    return m_localMatrix;
}
