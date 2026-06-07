#include "stdafx.h"
#include "Light.h"

LightObject::LightObject(LightType type) {
    m_lightData.lightType = static_cast<uint32_t>(type);
}

void LightObject::setIntensity(float intensity) {
    m_lightData.intensity = intensity;
}

void LightObject::setColor(const glm::vec3& color) {
    m_lightData.color = color;
}

void LightObject::render(RenderQueue& queue) {
    queue.addLight(m_lightData);
    GameObject::render(queue);
}

void LightObject::updateWorldMatrix() {
    GameObject::updateWorldMatrix();

    m_lightData.position = m_worldMatrix[3];
    m_lightData.direction = glm::normalize(glm::vec3(m_worldMatrix[2]));
}

DirectionalLight::DirectionalLight() : LightObject(LightType::Directional) {
    m_lightData.castShadow = TRUE;
}

PointLight::PointLight() : LightObject(LightType::Point) {
    m_lightData.castShadow = FALSE;
}

void PointLight::setRange(float range) {
    m_lightData.range = range;
}

SpotLight::SpotLight() : LightObject(LightType::Spot) {
    m_lightData.castShadow = FALSE;
}

void SpotLight::setRange(float range) {
    m_lightData.range = range;
}

void SpotLight::setSpotAngles(float inner, float outer) {
    m_lightData.spotInner = glm::cos(glm::radians(inner));
    m_lightData.spotOuter = glm::cos(glm::radians(outer));
}
