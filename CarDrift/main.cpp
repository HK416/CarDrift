#include "stdafx.h"
#include "Renderer.h"
#include "GameScene.h"
#include "TestbedScene.h"

const uint32_t DEF_WIDTH = 1280;
const uint32_t DEF_HEIGHT = 720;

void handleWindowSize(GLFWwindow* window, int width, int height) {
    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->setFramebufferResized();
}

void handleKeyInput(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    auto sceneMgr = renderer->getSceneManager();
    sceneMgr->dispatchEvent(KeyEvent{key, scancode, action, mods});
}

void handleMouseButton(GLFWwindow* window, int button, int action, int mods) {
    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    auto sceneMgr = renderer->getSceneManager();
    sceneMgr->dispatchEvent(MouseButtonEvent{button, action, mods});
}

void handleScroll(GLFWwindow* window, double xoffset, double yoffset) {
    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    auto sceneMgr = renderer->getSceneManager();
    sceneMgr->dispatchEvent(MouseScrollEvent{xoffset, yoffset});
}

void handleCursorPos(GLFWwindow* window, double xpos, double ypos) {
    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    auto sceneMgr = renderer->getSceneManager();
    sceneMgr->dispatchEvent(CursorPosEvent{xpos, ypos});
}

void handleCharInput(GLFWwindow* window, unsigned int codepoint) {
    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    auto sceneMgr = renderer->getSceneManager();
    sceneMgr->dispatchEvent(CharEvent{codepoint});
}

int main(int argc, char** argv) {
    if (glfwInit() == GLFW_FALSE) {
        spdlog::error("Failed to initialize GLFW");
        return EXIT_FAILURE;
    }

    if (glfwVulkanSupported() == GLFW_FALSE) {
        spdlog::error("Vulkan is not supported on this system");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(DEF_WIDTH, DEF_HEIGHT, "CarDrift", nullptr, nullptr);
    if (window == nullptr) {
        spdlog::error("Failed to create GLFW window");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    try {
        auto renderer = std::make_unique<Renderer>(window);
        SceneManager* sceneMgr = renderer->getSceneManager();
        sceneMgr->pushScene(std::make_unique<TestbedScene>(renderer.get(), sceneMgr));

        glfwSetWindowUserPointer(window, renderer.get());

        glfwSetKeyCallback(window, handleKeyInput);
        glfwSetMouseButtonCallback(window, handleMouseButton);
        glfwSetScrollCallback(window, handleScroll);
        glfwSetCursorPosCallback(window, handleCursorPos);
        glfwSetCharCallback(window, handleCharInput);
        glfwSetFramebufferSizeCallback(window, handleWindowSize);

        float lastTime = static_cast<float>(glfwGetTime());

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            float currentTime = static_cast<float>(glfwGetTime());
            float elapsedTimeSec = currentTime - lastTime;
            lastTime = currentTime;

            if (elapsedTimeSec > 0.25f) {
                elapsedTimeSec = 0.25f;
            }

            renderer->drawFrame(elapsedTimeSec);
        }
    }
    catch (std::exception& e) {
        spdlog::error(e.what());
    }
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}