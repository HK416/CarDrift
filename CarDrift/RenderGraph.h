#pragma once

class Mesh;
class Material;

// --- Lights ---
//
enum class LightType : uint32_t {
    Directional = 0,
    Point = 1,
    Spot = 2,
};

struct Light {
    glm::vec4 position; // xyz: Position, w: LightType
    glm::vec4 direction; // xyz: Direction, w: range
    glm::vec4 color; // rgb: Color, w: intensity
    glm::vec4 params; // x: spotInner, y: spotOuter, zw: padding
    
    int shadowIndex = -1; // Shadow Map Index (-1: None)
    uint32_t _padding0[3];
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
    uint32_t _padding1[2];

    static const uint32_t MAX_SHADOW_MAPS = 4;
    glm::mat4 shadowMatrices[MAX_SHADOW_MAPS];
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
