#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#define GLM_DORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/fmt/ostr.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <variant>
#include <vector>