#pragma once

class SceneManager;
class Renderer;
class RenderContext;
class RenderQueue;
class Mesh;
class Material;
class Texture;
class Shader;
class ShaderLayout;

// --- Events ---
//
struct KeyEvent { int key, scancode, action, mods; };
struct MouseButtonEvent { int button, action, mods; };
struct MouseScrollEvent { double xoffset, yoffset; };
struct CursorPosEvent { double xpos, ypos; };
struct CharEvent { unsigned int codepoint; };

using Event = std::variant<KeyEvent, MouseButtonEvent, MouseScrollEvent, CursorPosEvent, CharEvent>;

// --- Base Scene ---
//
class GameScene {
public:
    GameScene() = delete;
    GameScene(const GameScene&) = delete;
    GameScene& operator=(const GameScene&) = delete;
    GameScene(Renderer* renderer, SceneManager* manager);
    virtual ~GameScene() = default;

    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void onPause() {}
    virtual void onResume() {}

    virtual bool onEvent(const Event& event) { return false; }

    virtual void preUpdate(float elapsedTimeSec) {}
    virtual void update(float elapsedTimeSec) = 0;
    virtual void fixedUpdate(float fixedStep) {}
    virtual void postUpdate(float elapsedTimeSec) {}

    virtual void onPreRender(VkCommandBuffer commandBuffer) {}
    virtual void render(RenderQueue& queue, float alpha) = 0;
    virtual void onPostRender(VkCommandBuffer commandBuffer) {}

    virtual bool isOpaque() const { return true; }
    virtual bool isPausedBehind() const { return true; }
    virtual bool shouldClearDepth() const { return false; }

    void addMesh(const std::string& name, std::unique_ptr<Mesh> mesh);
    Mesh* getMesh(const std::string& name) const;

    void addMaterial(const std::string& name, std::unique_ptr<Material> material);
    Material* getMaterial(const std::string& name) const;
    
    void addTexture(const std::string& name, std::unique_ptr<Texture> texture);
    Texture* getTexture(const std::string& name) const;
    
    void addShader(const std::string& name, std::unique_ptr<Shader> shader);
    Shader* getShader(const std::string& name) const;

    void addShaderLayout(const std::string& name, std::unique_ptr<ShaderLayout> layout);
    ShaderLayout* getShaderLayout(const std::string& name) const;

protected:
    Renderer* m_renderer;
    SceneManager* m_manager;

    template<typename T>
    using ResourceCache = std::unordered_map<std::string, std::unique_ptr<T>>;

    ResourceCache<Mesh> m_meshes;
    ResourceCache<Material> m_materials;
    ResourceCache<Texture> m_textures;
    ResourceCache<Shader> m_shaders;
    ResourceCache<ShaderLayout> m_shaderLayouts;
};

// --- Manager ---
//
class SceneManager {
public:
    enum class ActionType { Push, Pop, Replace, Clear };

    struct SceneRequest {
        ActionType type;
        std::unique_ptr<GameScene> scene;
    };

public:
    SceneManager() = delete;
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager(Renderer* renderer);
    ~SceneManager();

    void pushScene(std::unique_ptr<GameScene> scene);
    void popScene();
    void replaceScene(std::unique_ptr<GameScene> scene);
    void clear();

    void update(float elapsedTimeSec);

    void prepareRenderQueue(uint32_t frameIndex, RenderQueue& queue);
    void renderShadowPass(VkCommandBuffer cmd, uint32_t frameIndex, RenderQueue& queue);
    void renderMainPass(VkCommandBuffer cmd, uint32_t frameIndex, RenderQueue& queue);

    void render(VkCommandBuffer commandBuffer, uint32_t frameIndex, VkExtent2D extent);
    void dispatchEvent(const Event& event);

    void setFixedUpdateStep(float step);
    void helperClearDepth(VkCommandBuffer commandBuffer);

    VkExtent2D getCurrentExtent() const { return m_currentExtent; }

private:
    void processPendingRequests();
    void clearImmediate();
    void executeRenderQueue(VkCommandBuffer cmd, uint32_t frameIndex, RenderQueue& queue);

private:
    Renderer* m_renderer;
    std::vector<std::unique_ptr<GameScene>> m_sceneStack;
    std::queue<SceneRequest> m_pendingRequests;

    float m_fixedUpdateStep = 1.0f / 60.0f;
    float m_fixedUpdateAccumulator = 0.0f;

    VkExtent2D m_currentExtent{0, 0};
};
