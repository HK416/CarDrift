#pragma once

class SceneManager;
class RenderContext;

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
    GameScene(RenderContext* context, SceneManager* manager);
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
    virtual void render(VkCommandBuffer commandBuffer, float alpha) = 0;

    virtual bool isOpaque() const { return true; }
    virtual bool isPausedBehind() const { return true; }
    virtual bool shouldClearDepth() const { return false; }

protected:
    RenderContext* m_context;
    SceneManager* m_manager;
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
    SceneManager(RenderContext* context);
    ~SceneManager();

    void pushScene(std::unique_ptr<GameScene> scene);
    void popScene();
    void replaceScene(std::unique_ptr<GameScene> scene);
    void clear();

    void update(float elapsedTimeSec);
    void render(VkCommandBuffer commandBuffer, VkExtent2D extent);
    void dispatchEvent(const Event& event);

    void setFixedUpdateStep(float step);
    void helperClearDepth(VkCommandBuffer commandBuffer);

    VkExtent2D getCurrentExtent() const { return m_currentExtent; }

private:
    void processPendingRequests();
    void clearImmediate();

private:
    RenderContext* m_context;
    std::vector<std::unique_ptr<GameScene>> m_sceneStack;
    std::queue<SceneRequest> m_pendingRequests;

    float m_fixedUpdateStep = 1.0f / 60.0f;
    float m_fixedUpdateAccumulator = 0.0f;

    VkExtent2D m_currentExtent{0, 0};
};
