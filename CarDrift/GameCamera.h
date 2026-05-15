#pragma once
#include "GameObject.h"

class RenderQueue;

class GameCamera : public GameObject {
public:
    GameCamera() = default;
    GameCamera(const GameCamera&) = delete;
    GameCamera& operator=(const GameCamera&) = delete;
    virtual ~GameCamera() = default;

    virtual const glm::mat4& getViewMatrix() const = 0;
    virtual const glm::mat4& getProjectionMatrix() const = 0;

    void applyToQueue(RenderQueue& queue);
};

class PerspectiveCamera : public GameCamera {
public:
    PerspectiveCamera();
    PerspectiveCamera(const PerspectiveCamera&) = delete;
    PerspectiveCamera& operator=(const PerspectiveCamera&) = delete;
    virtual ~PerspectiveCamera() = default;

    void setPerspective(float fovYDeg, float aspectRatio, float nearZ, float farZ);
    void setAspectRatio(float aspect);

    virtual const glm::mat4& getViewMatrix() const override;
    virtual const glm::mat4& getProjectionMatrix() const override;

protected:
    virtual void updateWorldMatrix() override;

protected:
    float m_fov = 45.0f;
    float m_aspectRatio = 1.0f;
    float m_nearZ = 0.1f;
    float m_farZ = 100.0f;

    glm::mat4 m_projectionMatrix{1.0f};
    glm::mat4 m_viewMatrix{1.0f};
};

class ThirdPersonCamera : public PerspectiveCamera {
public:
    ThirdPersonCamera() = default;
    ThirdPersonCamera(const ThirdPersonCamera&) = delete;
    ThirdPersonCamera& operator=(const ThirdPersonCamera&) = delete;
    virtual ~ThirdPersonCamera() = default;

    void setTarget(GameObject* target);
    void setFollowParams(float distance, float height, float smoothSpeed);
    void setLookAtOffset(float heightOffset);

protected:
    virtual void onUpdate(float elapsedTimeSec) override;

private:
    GameObject* m_target = nullptr;
    float m_distance = 5.0f;
    float m_height = 2.0f;
    float m_smoothSpeed = 10.0f;
    float m_lookAtHeightOffset = 1.0f;
};
