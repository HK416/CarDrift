#include "stdafx.h"
#include "GameScene.h"
#include "RenderGraph.h"
#include "Renderer.h"
#include "Shader.h"
#include "ShaderLayout.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

GameScene::GameScene(Renderer* renderer, SceneManager* manager) 
    : m_renderer(renderer), m_manager(manager) {}

SceneManager::SceneManager(Renderer* renderer) : m_renderer(renderer) {}

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

void SceneManager::render(
    VkCommandBuffer commandBuffer, uint32_t frameIndex, VkExtent2D extent
) {
    if (m_sceneStack.empty()) {
        return;
    }

    m_currentExtent = extent;
    RenderQueue queue;
    queue.clear();

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
        m_sceneStack[i]->render(queue, alpha);
    }

    m_renderer->updateGlobalBuffer(frameIndex, queue.getGlobalData());

    queue.sort();
    executeRenderQueue(commandBuffer, frameIndex, queue);

    for (size_t i = firstIndex; i < m_sceneStack.size(); ++i) {
        m_sceneStack[i]->onPostRender(commandBuffer);
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

void SceneManager::executeRenderQueue(
    VkCommandBuffer cmd, uint32_t frameIndex, RenderQueue& queue
) {
    const auto& opaqueItems = queue.getOpaqueItems();
    if (!opaqueItems.empty()) {
        Shader* lastShader = nullptr;
        Material* lastMaterial = nullptr;
        Mesh* lastMesh = nullptr;

        PushConstantData pcData;
        VkDescriptorSet globalSet = m_renderer->getGlobalDescriptorSet(frameIndex);

        for (const auto& item : opaqueItems) {
            if (!item.mesh || !item.material) {
                continue;
            }

            if (item.material->getShader() != lastShader) {
                lastShader = item.material->getShader();
                lastShader->bind(cmd);

                VkPipelineLayout pipelineLayout = lastShader->getLayout()->getPipelineLayout();
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &globalSet, 0, nullptr);
            }

            if (item.material != lastMaterial) {
                lastMaterial = item.material;
                lastMaterial->bind(cmd);
            }

            if (item.mesh != lastMesh) {
                lastMesh = item.mesh;
                pcData.attributeMask = item.mesh->getAttributeMask();

                
                VkPipelineLayout pipelineLayout = lastShader->getLayout()->getPipelineLayout();
                lastMesh->bindBuffers(cmd, pipelineLayout);
            }

            pcData.worldMatrix = item.worldMatrix;
            vkCmdPushConstants(
                cmd,
                lastShader->getLayout()->getPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(PushConstantData),
                &pcData
            );

            const auto& submesh = item.mesh->getSubMeshes()[item.submeshIndex];
            vkCmdDrawIndexed(cmd, submesh.indexCount, 1, submesh.indexStart, 0, 0);
        }
    }
}

void GameScene::addMesh(const std::string& name, std::unique_ptr<Mesh> mesh) {
    m_meshes[name] = std::move(mesh);
}

Mesh* GameScene::getMesh(const std::string& name) const {
    auto it = m_meshes.find(name);
    return (it != m_meshes.end()) ? it->second.get() : nullptr;
}

void GameScene::addMaterial(const std::string& name, std::unique_ptr<Material> material) {
    m_materials[name] = std::move(material);
}

Material* GameScene::getMaterial(const std::string& name) const {
    auto it = m_materials.find(name);
    return (it != m_materials.end()) ? it->second.get() : nullptr;
}

void GameScene::addTexture(const std::string& name, std::unique_ptr<Texture> texture) {
    m_textures[name] = std::move(texture);
}

Texture* GameScene::getTexture(const std::string& name) const {
    auto it = m_textures.find(name);
    return (it != m_textures.end()) ? it->second.get() : nullptr;
}

void GameScene::addShader(const std::string& name, std::unique_ptr<Shader> shader) {
    m_shaders[name] = std::move(shader);
}

Shader* GameScene::getShader(const std::string& name) const {
    auto it = m_shaders.find(name);
    return (it != m_shaders.end()) ? it->second.get() : nullptr;
}

void GameScene::addShaderLayout(const std::string& name, std::unique_ptr<ShaderLayout> layout) {
    m_shaderLayouts[name] = std::move(layout);
}

ShaderLayout* GameScene::getShaderLayout(const std::string& name) const {
    auto it = m_shaderLayouts.find(name);
    return (it != m_shaderLayouts.end()) ? it->second.get() : nullptr;
}
