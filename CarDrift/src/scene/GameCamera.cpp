#include "stdafx.h"
#include "GameCamera.h"
#include "RenderGraph.h"

void GameCamera::applyToQueue(RenderQueue& queue) {
    queue.setCamera(getViewMatrix(), getProjectionMatrix(), getTransform().getPosition());
}

PerspectiveCamera::PerspectiveCamera() {
    setPerspective(45.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
}

void PerspectiveCamera::setPerspective(
    float fovYDeg, float aspectRatio, float nearZ, float farZ
) {
    m_fov = fovYDeg;
    m_aspectRatio = aspectRatio;
    m_nearZ = nearZ;
    m_farZ = farZ;

    m_projectionMatrix = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearZ, m_farZ);
    m_projectionMatrix[1][1] *= -1.0f;
}

void PerspectiveCamera::setAspectRatio(float aspect) {
    setPerspective(m_fov, aspect, m_nearZ, m_farZ);
}

const glm::mat4& PerspectiveCamera::getViewMatrix() const {
    return m_viewMatrix;
}

const glm::mat4& PerspectiveCamera::getProjectionMatrix() const {
    return m_projectionMatrix;
}

void PerspectiveCamera::updateWorldMatrix() {
    GameObject::updateWorldMatrix();
    m_viewMatrix = glm::inverse(m_worldMatrix);
}

void ThirdPersonCamera::setTarget(GameObject* target) {
    m_target = target;
}

void ThirdPersonCamera::setFollowParams(
    float distance, float height, float smoothSpeed
) {
    m_distance = distance;
    m_height = height;
    m_smoothSpeed = smoothSpeed;
}

void ThirdPersonCamera::setLookAtOffset(float heightOffset) {
    m_lookAtHeightOffset = heightOffset;
}

void ThirdPersonCamera::onUpdate(float elapsedTimeSec) {
    if (!m_target) {
        return;
    }

    Transform& targetTransform = m_target->getTransform();
    Transform& myTransform = getTransform();

    glm::vec3 targetPos = targetTransform.getPosition();
    glm::mat4 targetMat = targetTransform.getLocalMatrix();

    glm::vec3 targetBackward = glm::normalize(glm::vec3(targetMat[2]));
    glm::vec3 targetUp = glm::normalize(glm::vec3(targetMat[1]));

    // Calculate Destination
    glm::vec3 desiredPosition = targetPos + (targetBackward * m_distance) + (targetUp * m_height);

    // Smooth moving
    glm::vec3 currentPos = myTransform.getPosition();
    glm::vec3 smoothedPos = glm::mix(currentPos, desiredPosition, m_smoothSpeed * elapsedTimeSec);
    myTransform.setPosition(smoothedPos);

    // Update Rotation (Look At Target)
    glm::vec3 lookAtTarget = targetPos + glm::vec3(0.0f, m_lookAtHeightOffset, 0.0f);

    glm::mat4 lookAtView = glm::lookAt(smoothedPos, lookAtTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat desiredRotation = glm::quat_cast(glm::inverse(lookAtView));

    myTransform.setRotation(desiredRotation);
}
