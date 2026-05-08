#include "stdafx.h"
#include "SceneBase.h"

GameScene::GameScene(RenderContext* context, SceneManager* manager) 
    : m_context(context), m_manager(manager) {}

SceneManager::SceneManager(RenderContext* context) : m_context(context) {}

SceneManager::~SceneManager() {
    clearImmediate();
}

void SceneManager::pushScene(std::unique_ptr<GameScene> scene) {
    m_pendingRequests.push({ActionType::Push, std::move(scene)});
}

void SceneManager::popScene() {
    m_pendingRequests.push({ActionType::Pop, nullptr});
}

void SceneManager::replaceScene(std::unique_ptr<GameScene> scene) {
    m_pendingRequests.push({ActionType::Replace, std::move(scene)});
}

void SceneManager::clear() {
    m_pendingRequests.push({ActionType::Clear, nullptr});
}

void SceneManager::update(float elapsedTimeSec) {
    processPendingRequests();
    if (m_sceneStack.empty()) {
        return;
    }

    size_t firstIndex = m_sceneStack.size() - 1;
    while (firstIndex > 0 && m_sceneStack[firstIndex]->isPausedBehind()) {
        firstIndex -= 1;
    }

    // 1. Fixed Update
    m_fixedUpdateAccumulator += elapsedTimeSec;
    while (m_fixedUpdateAccumulator >= m_fixedUpdateStep) {
        for (size_t i = firstIndex; i < m_sceneStack.size(); ++i) {
            m_sceneStack[i]->fixedUpdate(m_fixedUpdateStep);
        }
        m_fixedUpdateAccumulator -= m_fixedUpdateStep;
    }

    // 2. Update
    for (size_t i = firstIndex; i < m_sceneStack.size(); ++i) {
        m_sceneStack[i]->preUpdate(elapsedTimeSec);
        m_sceneStack[i]->update(elapsedTimeSec);
        m_sceneStack[i]->postUpdate(elapsedTimeSec);
    }
}

void SceneManager::render(VkCommandBuffer commandBuffer, VkExtent2D extent) {
    if (m_sceneStack.empty()) {
        return;
    }

    m_currentExtent = extent;

    size_t firstIndex = m_sceneStack.size() - 1;
    while (firstIndex > 0 && !m_sceneStack[firstIndex]->isOpaque()) {
        firstIndex -= 1;
    }

    float alpha = m_fixedUpdateAccumulator / m_fixedUpdateStep;
    for (size_t i = firstIndex; i < m_sceneStack.size(); ++i) {
        if (m_sceneStack[i]->shouldClearDepth()) {
            helperClearDepth(commandBuffer);
        }

        m_sceneStack[i]->onPreRender(commandBuffer);
        m_sceneStack[i]->render(commandBuffer, alpha);
    }
}

void SceneManager::dispatchEvent(const Event& event) {
    for (int i = static_cast<int>(m_sceneStack.size()) - 1; i >= 0; --i) {
        if (m_sceneStack[i]->onEvent(event) ||
            m_sceneStack[i]->isPausedBehind()) {
            break;
        }
    }
}

void SceneManager::setFixedUpdateStep(float step) {
    m_fixedUpdateStep = step;
}

void SceneManager::helperClearDepth(VkCommandBuffer commandBuffer) {
    VkClearAttachment clearAttachment = {};
    clearAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    clearAttachment.clearValue.depthStencil = {1.0f, 0};

    VkClearRect clearRect = {};
    clearRect.rect.extent = m_currentExtent;
    clearRect.layerCount = 1;

    vkCmdClearAttachments(commandBuffer, 1, &clearAttachment, 1, &clearRect);
}

void SceneManager::processPendingRequests() {
    while (!m_pendingRequests.empty()) {
        auto& request = m_pendingRequests.front();

        switch (request.type) {
            case ActionType::Push:
                if (!m_sceneStack.empty()) {
                    m_sceneStack.back()->onPause();
                }

                m_sceneStack.push_back(std::move(request.scene));
                m_sceneStack.back()->onEnter();
                break;
            case ActionType::Pop:
                if (!m_sceneStack.empty()) {
                    m_sceneStack.back()->onExit();
                    m_sceneStack.pop_back();

                    if (!m_sceneStack.empty()) {
                        m_sceneStack.back()->onResume();
                    }
                }
                break;
            case ActionType::Replace:
                if (!m_sceneStack.empty()) {
                    m_sceneStack.back()->onExit();
                    m_sceneStack.pop_back();
                }

                m_sceneStack.push_back(std::move(request.scene));
                m_sceneStack.back()->onEnter();
                break;
            case ActionType::Clear:
                clearImmediate();
                break;
        }

        m_pendingRequests.pop();
    }
}

void SceneManager::clearImmediate() {
    while (!m_sceneStack.empty()) {
        m_sceneStack.back()->onExit();
        m_sceneStack.pop_back();
    }
}
