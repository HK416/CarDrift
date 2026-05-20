#pragma once

class Mesh;
class Material;

// --- Light Data ---
//
struct Light {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    uint32_t lightType = 0;

    glm::vec3 direction{0.0f, 0.0f, 0.0f};
    float range = 0.0f;

    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 0.0f;

    float spotInner = 0.0f;
    float spotOuter = 0.0f;
    uint32_t castShadow = 0; // (None: 0)
    uint32_t _padding0;
};

// --- Global Data ---
//
struct GlobalData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 cameraPos;
    uint32_t _padding0;

    static const uint32_t MAX_LIGHTS = 16;
    Light lights[MAX_LIGHTS];

    uint32_t lightCount = 0;
    float ambientIntensity = 0.2f;
    float gamma = 2.2f;
    uint32_t cascadeCount = 0;

    glm::vec4 cascadeSplits;

    static const uint32_t MAX_SHADOW_MAPS = 4;
    glm::mat4 shadowMatrices[MAX_SHADOW_MAPS]{
        glm::mat4{1.0f},
        glm::mat4{1.0f},
        glm::mat4{1.0f},
        glm::mat4{1.0f},
    };
};

// --- Rendering Data ---
//
struct RenderItem {
    Mesh* mesh = nullptr;
    Material* material = nullptr;
    uint32_t submeshIndex = 0;
    glm::mat4 worldMatrix{1.0f};
    float sortDistance = 0.0f;
};

class RenderQueue {
public:
    void clear();

    void setCamera(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos);
    void addLight(const Light& light);
    void setShadowMatrix(uint32_t index, const glm::mat4& matrix);
    void setAmbient(float intensity);
    void setCascadeData(uint32_t count, const glm::vec4& splits);

    void addOpaque(const RenderItem& item);
    void addTransparent(const RenderItem& item);

    void sort();

    const std::vector<RenderItem>& getOpaqueItems() const { return m_opaqueItems; }
    const std::vector<RenderItem>& getTransparentItems() const { return m_transparentItems; }
    const GlobalData& getGlobalData() const { return m_globalData; }

private:
    GlobalData m_globalData;
    std::vector<RenderItem> m_opaqueItems;
    std::vector<RenderItem> m_transparentItems;
};
