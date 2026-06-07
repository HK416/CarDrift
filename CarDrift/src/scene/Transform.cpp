#include "stdafx.h"
#include "Transform.h"

void Transform::setFromMatrix(const glm::mat4& matrix) {
    m_position = glm::vec3(matrix[3]);

    m_scale = glm::vec3(
        glm::length(glm::vec3(matrix[0])),
        glm::length(glm::vec3(matrix[1])),
        glm::length(glm::vec3(matrix[2]))
    );

    glm::mat3 rotMat;

    if (m_scale.x > 0.00001f) {
        rotMat[0] = glm::vec3(matrix[0]) / m_scale.x;
    } else {
        rotMat[0] = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    if (m_scale.y > 0.00001f) {
        rotMat[1] = glm::vec3(matrix[1]) / m_scale.y;
    } else {
        rotMat[1] = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    if (m_scale.z > 0.00001f) {
        rotMat[2] = glm::vec3(matrix[2]) / m_scale.z;
    } else {
        rotMat[2] = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    m_rotation = glm::quat_cast(rotMat);
    setDirty();
}

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
