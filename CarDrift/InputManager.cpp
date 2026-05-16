#include "stdafx.h"
#include "InputManager.h"

GLFWwindow* InputManager::s_window = nullptr;

void InputManager::init(GLFWwindow* window) {
    s_window = window;
}

void InputManager::shutdown() {
    s_window = nullptr;
}

bool InputManager::getKey(int glfwKeyCode) {
    if (!s_window) {
        return false;
    }

    return glfwGetKey(s_window, glfwKeyCode) == GLFW_PRESS ||
           glfwGetKey(s_window, glfwKeyCode) == GLFW_REPEAT;
}
