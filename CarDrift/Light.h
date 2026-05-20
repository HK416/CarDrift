#pragma once
#include "GameObject.h"
#include "RenderGraph.h"

enum class LightType : uint32_t {
    Directional = 0,
    Point = 1,
    Spot = 2,
};

class LightObject : public GameObject {
public:
    LightObject() = delete;
    LightObject(const LightObject&) = delete;
    LightObject& operator=(const LightObject&) = delete;
    LightObject(LightType type);
    virtual ~LightObject() = default;

    const Light& getLightData() const { return m_lightData; }

    void setIntensity(float intensity);
    void setColor(const glm::vec3& color);

    virtual void render(RenderQueue& queue) override;

protected:
    virtual void updateWorldMatrix() override;

protected:
    Light m_lightData;
};

class DirectionalLight : public LightObject {
public:
    DirectionalLight();
    DirectionalLight(const DirectionalLight&) = delete;
    DirectionalLight& operator=(const DirectionalLight&) = delete;
};

class PointLight : public LightObject {
public:
    PointLight();
    PointLight(const PointLight&) = delete;
    PointLight& operator=(const PointLight&) = delete;

    void setRange(float range);
};

class SpotLight : public LightObject {
public:
    SpotLight();
    SpotLight(const SpotLight&) = delete;
    SpotLight& operator=(const SpotLight&) = delete;

    void setRange(float range);
    void setSpotAngles(float inner, float outer);
};
