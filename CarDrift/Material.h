#pragma once

class RenderContext;
class Shader;
class Texture;

class Material {
public:
    Material() = delete;
    Material(const Material&) = delete;
    Material(RenderContext* context, std::shared_ptr<Shader> shader);
    virtual ~Material() = default;

    virtual void bind(VkCommandBuffer cmd);

    std::shared_ptr<Shader> getShader() const { return m_shader; }

protected:
    virtual void updateDescriptorSet() = 0;

protected:
    RenderContext* m_context; // 소유하지 않는 클래스 맴버
    std::shared_ptr<Shader> m_shader;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
};

class StandardMaterial : public Material {
public:
    StandardMaterial() = delete;
    StandardMaterial(const StandardMaterial&) = delete;
    StandardMaterial(RenderContext* context, std::shared_ptr<Shader> shader);
    virtual ~StandardMaterial();

    StandardMaterial& setAlbedo(const glm::vec4& color);
    StandardMaterial& setMetallic(float m);
    StandardMaterial& setRoughness(float r);

    StandardMaterial& setAlbedoMap(std::shared_ptr<Texture> tex);
    StandardMaterial& setNormalMap(std::shared_ptr<Texture> tex);
    StandardMaterial& setMetallicRoughnessMap(std::shared_ptr<Texture> tex);

    virtual void bind(VkCommandBuffer cmd) override;

protected:
    virtual void updateDescriptorSet() override;

private:
    struct PBRMaterialParams {
        glm::vec4 albedoFactor{1.0f, 1.0f, 1.0f, 1.0f};
        float metallicFactor{1.0f};
        float roughnessFactor{1.0f};
        float aoFactor{1.0f};
        uint32_t _padding0;
    };

    PBRMaterialParams m_params;

    std::shared_ptr<Texture> m_albedoMap;
    std::shared_ptr<Texture> m_normalMap;
    std::shared_ptr<Texture> m_mrMap;

    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;

    bool m_dirty = true;
};