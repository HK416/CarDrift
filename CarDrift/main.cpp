#include "stdafx.h"
#include "Renderer.h"

const uint32_t DEF_WIDTH = 1280;
const uint32_t DEF_HEIGHT = 720;

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
        auto temp = new RenderContext(window);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }

        delete temp;
    }
    catch (std::exception& e) {
        spdlog::error(e.what());
    }
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}