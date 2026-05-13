#include "stdafx.h"
#include "RenderGraph.h"
#include "Material.h"
#include "Shader.h"

void RenderQueue::clear() {
    m_opaqueItems.clear();
    m_transparentItems.clear();
    m_globalData.lightCount = 0;
    for (uint32_t i = 0; i < GlobalData::MAX_SHADOW_MAPS; ++i) {
        m_globalData.shadowMatrices[i] = glm::mat4(1.0f);
    }
}

void RenderQueue::setCamera(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos) {
    m_globalData.view = view;
    m_globalData.proj = proj;
    m_globalData.cameraPos = glm::vec4(pos, 1.0f);
}

void RenderQueue::addLight(const Light& light) {
    if (m_globalData.lightCount < GlobalData::MAX_LIGHTS) {
        m_globalData.lights[m_globalData.lightCount] = light;
        m_globalData.lightCount += 1;
    }
}

void RenderQueue::setShadowMatrix(uint32_t index, const glm::mat4& matrix) {
    if (index < GlobalData::MAX_SHADOW_MAPS) {
        m_globalData.shadowMatrices[index] = matrix;
    }
}

void RenderQueue::setAmbient(float intensity) {
    m_globalData.ambientIntensity = intensity;
}

void RenderQueue::addOpaque(const RenderItem& item) {
    m_opaqueItems.push_back(item);
}

void RenderQueue::addTransparent(const RenderItem& item) {
    m_transparentItems.push_back(item);
}

void RenderQueue::sort() {
    std::sort(
        m_opaqueItems.begin(),
        m_opaqueItems.end(),
        [](const RenderItem& a, const RenderItem& b) {
            if (a.material != b.material) {
                if (a.material->getShader() != b.material->getShader()) {
                    return a.material->getShader() < b.material->getShader();
                }
                return a.material < b.material;
            }
            return a.sortDistance < b.sortDistance;
        }
    );

    std::sort(
        m_transparentItems.begin(),
        m_transparentItems.end(),
        [](const RenderItem& a, const RenderItem& b) {
            return a.sortDistance > b.sortDistance;
        }
    );
}
