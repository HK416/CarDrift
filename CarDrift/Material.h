#pragma once

class RenderContext;
class Shader;
class Texture;

class Material {
public:
    Material() = delete;
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(RenderContext* context, Shader* shader, bool transparent = false);
    virtual ~Material() = default;

    virtual void bind(VkCommandBuffer cmd);

    Shader* getShader() const { return m_shader; }
    bool isTransparent() const { return m_isTransparent; }

protected:
    virtual void updateDescriptorSet() = 0;

protected:
    RenderContext* m_context; // 소유하지 않는 클래스 맴버
    Shader* m_shader;         // 소유하지 않는 클래스 맴버 (장면이 관리함)
    
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    bool m_isTransparent = false;
};

class StandardMaterial : public Material {
public:
    StandardMaterial() = delete;
    StandardMaterial(const StandardMaterial&) = delete;
    StandardMaterial& operator=(const StandardMaterial&) = delete;
    StandardMaterial(RenderContext* context, Shader* shader);
    virtual ~StandardMaterial();

    StandardMaterial& setAlbedo(const glm::vec4& color);
    StandardMaterial& setMetallic(float m);
    StandardMaterial& setRoughness(float r);

    StandardMaterial& setAlbedoMap(Texture* tex);
    StandardMaterial& setNormalMap(Texture* tex);
    StandardMaterial& setMetallicRoughnessMap(Texture* tex);
    StandardMaterial& setAOMap(Texture* tex);

    virtual void bind(VkCommandBuffer cmd) override;

protected:
    virtual void updateDescriptorSet() override;

private:
    struct PBRMaterialParams {
        glm::vec4 albedoFactor{1.0f, 1.0f, 1.0f, 1.0f};
        float roughnessFactor{1.0f};
        float metallicFactor{1.0f};
        float alphaCutoff{0.5f};
        float aoFactor{1.0f};
    };

    PBRMaterialParams m_params;

    Texture* m_albedoMap; // 소유하지 않는 클래스 맴버 (장면 또는 엔진이 관리함)
    Texture* m_normalMap; // 소유하지 않는 클래스 맴버 (장면 또는 엔진이 관리함)
    Texture* m_mrMap;     // 소유하지 않는 클래스 맴버 (장면 또는 엔진이 관리함)
    Texture* m_aoMap;     // 소유하지 않는 클래스 맴버 (장면 또는 엔진이 관리함)

    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;

    bool m_dirty = true;
};