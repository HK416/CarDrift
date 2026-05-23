#pragma once

class GameObject;
class GameScene;
class RenderContext;

class GltfLoader {
public:
    static GameObject* load(
        const std::filesystem::path& path,
        GameScene* scene,
        RenderContext* context,
        VkCommandBuffer cmd
    );

private:
    static void loadTextures(
        fastgltf::Asset& asset,
        GameScene* scene,
        RenderContext* context,
        VkCommandBuffer cmd
    );

    static void loadMaterials(
        const std::filesystem::path& path,
        fastgltf::Asset& asset,
        GameScene* scene,
        RenderContext* context
    );

    static void loadMeshes(
        const std::filesystem::path& path,
        fastgltf::Asset& asset,
        GameScene* scene,
        RenderContext* context,
        VkCommandBuffer cmd
    );

    static GameObject* loadNode(
        const std::filesystem::path& path,
        size_t nodeIndex,
        fastgltf::Asset& asset,
        GameScene* scene,
        GameObject* parent,
        std::vector<GameObject*>& nodeMap
    );
};
