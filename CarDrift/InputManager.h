#pragma once

class InputManager {
public:
    static void init(GLFWwindow* window);
    static void shutdown();

    static bool getKey(int glfwKeyCode);

private:
    static GLFWwindow* s_window;
};
