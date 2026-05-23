#include "stdafx.h"
#include "GltfLoader.h"
#include "GameScene.h"
#include "GameObject.h"
#include "Renderer.h"
#include "Texture.h"
#include "Material.h"
#include "Mesh.h"

GameObject* GltfLoader::load(
    const std::filesystem::path& path,
    GameScene* scene,
    RenderContext* context,
    VkCommandBuffer cmd
) {
    fastgltf::Parser parser;
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (data.error() != fastgltf::Error::None) {
        throw std::runtime_error(std::format("Failed to load glTF data buffer from path: {}", path.string()));
    }

    constexpr auto options = fastgltf::Options::LoadExternalBuffers |
                             fastgltf::Options::LoadExternalImages;
    auto assetResult = parser.loadGltf(data.get(), path.parent_path(), options);

    if (assetResult.error() != fastgltf::Error::None) {
        throw std::runtime_error(std::format("Failed to parse glTF asset. Error code: {}", static_cast<int>(assetResult.error())));
    }

    fastgltf::Asset& asset = assetResult.get();
    loadTextures(asset, scene, context, cmd);
    loadMaterials(path, asset, scene, context);
    loadMeshes(path, asset, scene, context, cmd);

    auto rootObject = std::make_unique<GameObject>();
    rootObject->setName(path.filename().string() + "_Root");

    std::vector<GameObject*> nodeMap(asset.nodes.size(), nullptr);

    size_t sceneIndex = asset.defaultScene.value_or(0);
    if (sceneIndex < asset.scenes.size()) {
        for (size_t nodeIndex : asset.scenes[sceneIndex].nodeIndices) {
            auto childNode = loadNode(path, nodeIndex, asset, scene, rootObject.get(), nodeMap);
            rootObject->addChild(childNode);
        }
    }

    for (size_t i = 0; i < asset.nodes.size(); ++i) {
        if (!nodeMap[i]) {
            continue;
        }

        const auto& node = asset.nodes[i];
        if (node.skinIndex.has_value()) {
            size_t skinIdx = node.skinIndex.value();
            if (skinIdx >= asset.skins.size()) {
                continue;
            }

            const auto& skin = asset.skins[skinIdx];

            // --- Extract Bone Nodes ---
            std::vector<GameObject*> bones;
            bones.reserve(skin.joints.size());
            for (size_t jointIdx : skin.joints) {
                bones.emplace_back(nodeMap[jointIdx]);
            }

            // --- Extract Inverse Bindpose Matrices ----
            std::vector<glm::mat4> inverseBindposeMatrices(skin.joints.size(), glm::mat4(1.0f));
            if (skin.inverseBindMatrices.has_value()) {
                const auto& accessor = asset.accessors[skin.inverseBindMatrices.value()];
                fastgltf::iterateAccessorWithIndex<glm::mat4>(
                    asset, accessor, [&](glm::mat4 m, size_t idx) {
                        if (idx < inverseBindposeMatrices.size()) {
                            inverseBindposeMatrices[idx] = m;
                        }
                    }
                );
            }

            auto skinnedMesh = dynamic_cast<SkinnedMeshObject*>(nodeMap[i]);
            if (skinnedMesh) {
                skinnedMesh->setBones(bones, inverseBindposeMatrices);
            }
        }
    }

    GameObject* res = rootObject.get(); 
    scene->addRootObject(res);
    scene->addGameObject(std::move(rootObject));
    return res;
}

struct KtxTextureDeleter {
    void operator()(ktxTexture* tex) const {
        if (tex) {
            ktxTexture_Destroy(tex);
        }
    }
};
using KtxTextureUniquePtr = std::unique_ptr<ktxTexture, KtxTextureDeleter>;

void GltfLoader::loadTextures(
    fastgltf::Asset& asset,
    GameScene* scene,
    RenderContext* context,
    VkCommandBuffer cmd
) {
    std::vector<bool> isTextureSRGB(asset.textures.size(), false);
    for (const auto& mat : asset.materials) {
        if (mat.pbrData.baseColorTexture.has_value()) {
            size_t idx = mat.pbrData.baseColorTexture.value().textureIndex;
            if (idx < isTextureSRGB.size()) {
                isTextureSRGB[idx] = true;
            }
        }
        if (mat.emissiveTexture.has_value()) {
            size_t idx = mat.emissiveTexture.value().textureIndex;
            if (idx < isTextureSRGB.size()) {
                isTextureSRGB[idx] = true;
            }
        }
    }

    for (size_t i = 0; i < asset.textures.size(); ++i) {
        const auto& gltfTex = asset.textures[i];
        bool isSRGB = isTextureSRGB[i];

        std::string texName(gltfTex.name);
        if (scene->getTexture(texName) != nullptr) {
            continue;
        }

        spdlog::info("Loading texture: {} (sRGB: {})", texName, isSRGB);

        if (gltfTex.basisuImageIndex.has_value()) {
            size_t imageIndex = gltfTex.basisuImageIndex.value();
            if (imageIndex >= asset.images.size()) continue;

            const auto& image = asset.images[imageIndex];

            std::visit(
                fastgltf::visitor{
                    [&](const auto& arg) {
                        throw std::runtime_error("Unsupported image source type for Basis texture");
                    },
                    [&](const fastgltf::sources::BufferView& view) {
                        if (view.bufferViewIndex >= asset.bufferViews.size()) return;
                        auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                        if (bufferView.bufferIndex >= asset.buffers.size()) return;
                        auto& buffer = asset.buffers[bufferView.bufferIndex];

                        std::visit(
                            [&](auto& arr) {
                                if constexpr (requires { arr.bytes; }) {
                                    const uint8_t* rawBufferData = reinterpret_cast<const uint8_t*>(arr.bytes.data());
                                    const uint8_t* data = rawBufferData + bufferView.byteOffset;
                                    size_t size = bufferView.byteLength;

                                    ktxTexture* kTextureRaw = nullptr;
                                    KTX_error_code result = ktxTexture_CreateFromMemory(
                                        data,
                                        size,
                                        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                        &kTextureRaw
                                    );

                                    if (result != KTX_SUCCESS) {
                                        throw std::runtime_error(std::format("Failed to create KTX texture from memory. KTX error code: {}", static_cast<int>(result)));
                                    }

                                    KtxTextureUniquePtr kTexture(kTextureRaw);

                                    if (ktxTexture_NeedsTranscoding(kTexture.get())) {
                                        ktxTexture2* k2 = reinterpret_cast<ktxTexture2*>(kTexture.get());
                                        result = ktxTexture2_TranscodeBasis(k2, KTX_TTF_BC7_RGBA, 0);
                                        if (result != KTX_SUCCESS) {
                                            throw std::runtime_error(std::format("Failed to transcode Basis texture to BC7 format. KTX error code: {}", static_cast<int>(result)));
                                        }
                                    }

                                    std::vector<SubresourceData> subresources;
                                    uint32_t numLayers = std::max(1u, kTexture->numLayers);
                                    uint32_t numLevels = std::max(1u, kTexture->numLevels);

                                    for (uint32_t layer = 0; layer < numLayers; ++layer) {
                                        for (uint32_t mip = 0; mip < numLevels; ++mip) {
                                            size_t offset = 0;
                                            ktxTexture_GetImageOffset(kTexture.get(), mip, layer, 0, &offset);
                                            size_t size = ktxTexture_GetImageSize(kTexture.get(), mip);

                                            uint32_t mipWidth = std::max(1u, kTexture->baseWidth >> mip);
                                            uint32_t mipHeight = std::max(1u, kTexture->baseHeight >> mip);

                                            subresources.push_back({
                                                static_cast<uint32_t>(offset),
                                                static_cast<uint32_t>(size),
                                                mipWidth,
                                                mipHeight,
                                                mip,
                                                layer
                                            });
                                        }
                                    }

                                    VkFormat vkFormat = static_cast<VkFormat>(ktxTexture_GetVkFormat(kTexture.get()));

                                    MemoryTextureBuilder builder;
                                    builder.setRawDataWithSubresources(
                                        ktxTexture_GetData(kTexture.get()),
                                        ktxTexture_GetDataSize(kTexture.get()),
                                        kTexture->baseWidth,
                                        kTexture->baseHeight,
                                        numLevels,
                                        numLayers,
                                        vkFormat,
                                        subresources
                                    );
                                    scene->addTexture(texName, builder.build(context, cmd));
                                } else {
                                    throw std::runtime_error("Unsupported buffer data type for Basis texture");
                                }
                            },
                            buffer.data
                        );
                    }
                },
                image.data
            );

            continue;
        }

        if (gltfTex.imageIndex.has_value()) {
            size_t imageIndex = gltfTex.imageIndex.value();
            if (imageIndex >= asset.images.size()) continue;

            const auto& image = asset.images[imageIndex];

            std::visit(
                fastgltf::visitor{
                    [&](const auto& arg) { 
                         throw std::runtime_error("Unsupported image source type for standard texture");
                    },
                    [&](const fastgltf::sources::URI& uri) {
                        CommonTextureBuilder builder;
                        builder.setFile(uri.uri.path(), isSRGB);
                        scene->addTexture(texName, builder.build(context, cmd));
                    },
                    [&](const fastgltf::sources::Array& arr) {
                        MemoryTextureBuilder builder;
                        builder.setEncodedData(reinterpret_cast<const uint8_t*>(arr.bytes.data()), arr.bytes.size(), isSRGB);
                        scene->addTexture(texName, builder.build(context, cmd));
                    },
                    [&](const fastgltf::sources::Vector& vec) {
                        MemoryTextureBuilder builder;
                        builder.setEncodedData(reinterpret_cast<const uint8_t*>(vec.bytes.data()), vec.bytes.size(), isSRGB);
                        scene->addTexture(texName, builder.build(context, cmd));
                    },
                    [&](const fastgltf::sources::ByteView& view) {
                        MemoryTextureBuilder builder;
                        builder.setEncodedData(reinterpret_cast<const uint8_t*>(view.bytes.data()), view.bytes.size(), isSRGB);
                        scene->addTexture(texName, builder.build(context, cmd));
                    },
                    [&](const fastgltf::sources::BufferView& view) {
                        if (view.bufferViewIndex >= asset.bufferViews.size()) return;
                        auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                        if (bufferView.bufferIndex >= asset.buffers.size()) return;
                        auto& buffer = asset.buffers[bufferView.bufferIndex];

                        std::visit(
                            [&](const auto& arr) {
                                if constexpr (requires { arr.bytes; }) {
                                    const uint8_t* data = reinterpret_cast<const uint8_t*>(arr.bytes.data()) + bufferView.byteOffset;
                                    MemoryTextureBuilder builder;
                                    builder.setEncodedData(data, bufferView.byteLength, isSRGB);
                                    scene->addTexture(texName, builder.build(context, cmd));
                                }
                            },
                            buffer.data
                        );
                    }
                },
                image.data
            );
        }
    }
}

void GltfLoader::loadMaterials(
    const std::filesystem::path& path,
    fastgltf::Asset& asset,
    GameScene* scene,
    RenderContext* context
) {
    for (size_t i = 0; i < asset.materials.size(); ++i) {
        std::string matName = std::format("{}_mat_{}", path.filename().string(), i);
        if (scene->getMaterial(matName) != nullptr) {
            continue;
        }

        const auto& gltfMat = asset.materials[i];

        Shader* targetShader = nullptr;
        Shader* targetShaderShadow = nullptr;

        switch (gltfMat.alphaMode) {
            case fastgltf::AlphaMode::Opaque:
                targetShader = scene->getShader("StandardOpaque");
                targetShaderShadow = scene->getShader("StandardShadow");
                break;
            case fastgltf::AlphaMode::Mask:
                targetShader = scene->getShader("StandardCutoff");
                targetShaderShadow = scene->getShader("StandardShadowCutoff");
                break;
            case fastgltf::AlphaMode::Blend:
                targetShader = scene->getShader("StandardTransparent");
                break;
        }

        if (!targetShader) {
            throw std::runtime_error("TODO!");
        }

        auto material = std::make_unique<StandardMaterial>(context, targetShader, targetShaderShadow);

        const auto& pbr = gltfMat.pbrData;
        material->setAlbedo(
            glm::vec4(
                pbr.baseColorFactor[0],
                pbr.baseColorFactor[1],
                pbr.baseColorFactor[2],
                pbr.baseColorFactor[3]
            )
        );
        material->setMetallic(static_cast<float>(pbr.metallicFactor));
        material->setRoughness(static_cast<float>(pbr.roughnessFactor));

        if (gltfMat.alphaMode == fastgltf::AlphaMode::Mask) {
            material->setAlphaCutoff(static_cast<float>(gltfMat.alphaCutoff));
        }

        if (pbr.baseColorTexture.has_value()) {
            const auto& gltfTex = asset.textures[pbr.baseColorTexture.value().textureIndex];
            std::string texName(gltfTex.name);
            material->setAlbedoMap(scene->getTexture(texName));
        }

        if (pbr.metallicRoughnessTexture.has_value()) {
            const auto& gltfTex = asset.textures[pbr.metallicRoughnessTexture.value().textureIndex];
            std::string texName(gltfTex.name);
            material->setMetallicRoughnessMap(scene->getTexture(texName));
        }

        if (gltfMat.normalTexture.has_value()) {
            const auto& gltfTex = asset.textures[gltfMat.normalTexture.value().textureIndex];
            std::string texName(gltfTex.name);
            material->setNormalMap(scene->getTexture(texName));
        }

        if (gltfMat.occlusionTexture.has_value()) {
            const auto& gltfTex = asset.textures[gltfMat.occlusionTexture.value().textureIndex];
            std::string texName(gltfTex.name);
            material->setAOMap(scene->getTexture(texName));
        }

        scene->addMaterial(matName, std::move(material));
    }
}

void GltfLoader::loadMeshes(
    const std::filesystem::path& path,
    fastgltf::Asset& asset,
    GameScene* scene,
    RenderContext* context,
    VkCommandBuffer cmd
) {
    for (size_t i = 0; i < asset.meshes.size(); ++i) {
        std::string meshName = std::format("{}_mesh_{}", path.filename().string(), i);
        if (scene->getMesh(meshName) != nullptr) {
            continue;
        }

        const auto& gltfMesh = asset.meshes[i];
        MeshBuilder builder;

        std::vector<glm::vec3> allPositions;
        std::vector<glm::vec3> allNormals;
        std::vector<glm::vec4> allTangents;
        std::vector<glm::vec2> allTexcoords0;
        std::vector<glm::vec2> allTexcoords1;
        std::vector<glm::vec4> allColors;
        std::vector<glm::ivec4> allJointIndices;
        std::vector<glm::vec4> allJointWeights;
        std::vector<uint32_t> allIndices;

        uint32_t vertexOffset = 0;
        uint32_t indexOffset = 0;

        for (const auto& primitive : gltfMesh.primitives) {
            // Position
            auto posIt = primitive.findAttribute("POSITION");
            if (posIt != primitive.attributes.end()) {
                const auto& accessor = asset.accessors[posIt->accessorIndex];
                size_t startSize = allPositions.size();

                allPositions.resize(startSize + accessor.count);
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    asset, accessor, [&](glm::vec3 pos, size_t idx) {
                        allPositions[startSize + idx] = pos;
                    }
                );
            }

            // Normal
            auto normIt = primitive.findAttribute("NORMAL");
            if (normIt != primitive.attributes.end()) {
                const auto& accessor = asset.accessors[normIt->accessorIndex];
                size_t startSize = allNormals.size();

                allNormals.resize(startSize + accessor.count);
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    asset, accessor, [&](glm::vec3 norm, size_t idx) {
                        allNormals[startSize + idx] = norm;
                    }
                );
            }

            // Tangent
            auto tanIt = primitive.findAttribute("TANGENT");
            if (tanIt != primitive.attributes.end()) {
                const auto& accessor = asset.accessors[tanIt->accessorIndex];
                size_t startSize = allTangents.size();

                allTangents.resize(startSize + accessor.count);
                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                    asset, accessor, [&](glm::vec4 tan, size_t idx) {
                        allTangents[startSize + idx] = tan;
                    }
                );
            }

            // Texcoord_0
            auto uvIt = primitive.findAttribute("TEXCOORD_0");
            if (uvIt != primitive.attributes.end()) {
                const auto& accessor = asset.accessors[uvIt->accessorIndex];
                size_t startSize = allTexcoords0.size();

                allTexcoords0.resize(startSize + accessor.count);
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                    asset, accessor, [&](glm::vec2 uv, size_t idx) {
                        allTexcoords0[startSize + idx] = uv;
                    }
                );
            }

            // Texcoord_1
            uvIt = primitive.findAttribute("TEXCOORD_1");
            if (uvIt != primitive.attributes.end()) {
                const auto& accessor = asset.accessors[uvIt->accessorIndex];
                size_t startSize = allTexcoords1.size();

                allTexcoords1.resize(startSize + accessor.count);
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                    asset, accessor, [&](glm::vec2 uv, size_t idx) {
                        allTexcoords1[startSize + idx] = uv;
                    }
                );
            }

            // Color
            auto colorIt = primitive.findAttribute("COLOR_0");
            if (colorIt != primitive.attributes.end()) {
                const auto& accessor = asset.accessors[colorIt->accessorIndex];
                size_t startSize = allColors.size();

                allColors.resize(startSize + accessor.count);
                if (accessor.type == fastgltf::AccessorType::Vec3) {
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(
                        asset, accessor, [&](glm::vec3 color, size_t idx) {
                            allColors[startSize + idx] = glm::vec4(color.r, color.g, color.b, 1.0f);
                        }
                    );
                } else {
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(
                        asset, accessor, [&](glm::vec4 color, size_t idx) {
                            allColors[startSize + idx] = color;
                        }
                    );
                }
            }

            // Joints
            auto jointsIt = primitive.findAttribute("JOINTS_0");
            if (jointsIt != primitive.attributes.end()) {
                const auto& accessor = asset.accessors[jointsIt->accessorIndex];
                size_t startSize = allJointIndices.size();

                allJointIndices.resize(startSize + accessor.count);
                fastgltf::iterateAccessorWithIndex<glm::uvec4>(
                    asset, accessor, [&](glm::uvec4 joint, size_t idx) {
                        allJointIndices[startSize + idx] = glm::ivec4(joint.x, joint.y, joint.z, joint.w);
                    }
                );
            }

            // Weights
            auto weightIt = primitive.findAttribute("WEIGHTS_0");
            if (weightIt != primitive.attributes.end()) {
                const auto& accessor = asset.accessors[weightIt->accessorIndex];
                size_t startSize = allJointWeights.size();
                
                allJointWeights.resize(startSize + accessor.count);
                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                    asset, accessor, [&](glm::vec4 weight, size_t idx) {
                        allJointWeights[startSize + idx] = weight;
                    }
                );
            }

            // Index
            if (primitive.indicesAccessor.has_value()) {
                const auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
                size_t startSize = allIndices.size();

                allIndices.resize(startSize + accessor.count);
                fastgltf::iterateAccessorWithIndex<uint32_t>(
                    asset, accessor, [&](uint32_t val, size_t idx) {
                        allIndices[startSize + idx] = val;
                    }
                );

                builder.addSubMesh(indexOffset, static_cast<uint32_t>(accessor.count));
                indexOffset += static_cast<uint32_t>(accessor.count);
            }

            if (posIt != primitive.attributes.end()) {
                vertexOffset += static_cast<uint32_t>(asset.accessors[posIt->accessorIndex].count);
            }
        }

        if (!allPositions.empty())
            builder.setPosition(allPositions);
        if (!allNormals.empty())
            builder.setNormals(allNormals);
        if (!allTangents.empty())
            builder.setTangents(allTangents);
        if (!allTexcoords0.empty())
            builder.setTexcoord0(allTexcoords0);
        if (!allTexcoords1.empty())
            builder.setTexcoord1(allTexcoords1);
        if (!allJointIndices.empty())
            builder.setJointIndices(allJointIndices);
        if (!allJointWeights.empty())
            builder.setJointWeights(allJointWeights);
        if (!allIndices.empty())
            builder.setIndices(allIndices);

        VkDescriptorSetLayout layout = context->getGeometryDescriptorSetLayout();
        scene->addMesh(meshName, builder.build(context, cmd, layout));
    }
}

GameObject* GltfLoader::loadNode(
    const std::filesystem::path& path,
    size_t nodeIndex,
    fastgltf::Asset& asset,
    GameScene* scene,
    GameObject* parent,
    std::vector<GameObject*>& nodeMap
) {
    const auto& node = asset.nodes[nodeIndex];
    std::unique_ptr<GameObject> currentObject = nullptr;

    if (node.meshIndex.has_value()) {
        size_t meshIdx = node.meshIndex.value();
        std::string meshName = std::format("{}_mesh_{}", path.filename().string(), meshIdx);
        Mesh* mesh = scene->getMesh(meshName);

        std::vector<Material*> materials;
        if (meshIdx < asset.meshes.size()) {
            const auto& gltfMesh = asset.meshes[meshIdx];
            for (const auto& primitive : gltfMesh.primitives) {
                if (primitive.materialIndex.has_value()) {
                    size_t matIdx = primitive.materialIndex.value();
                    std::string matName = std::format("{}_mat_{}", path.filename().string(), matIdx);
                    materials.push_back(scene->getMaterial(matName));
                } else {
                    materials.push_back(nullptr);
                }
            }
        }

        if (node.skinIndex.has_value()) {
            currentObject = std::make_unique<SkinnedMeshObject>(mesh, materials);
        } else {
            currentObject = std::make_unique<MeshObject>(mesh, materials);
        }
    } else {
        currentObject = std::make_unique<GameObject>();
    }

    if (parent) {
        parent->addChild(currentObject.get());
    }

    auto& transform = currentObject->getTransform();
    if (const auto& trs = std::get_if<fastgltf::TRS>(&node.transform)) {
        transform.setPosition(glm::vec3(trs->translation[0], trs->translation[1], trs->translation[2]));
        transform.setRotation(glm::quat(trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]));
        transform.setScale(glm::vec3(trs->scale[0], trs->scale[1], trs->scale[2]));
    }
    else if (const auto* mat = std::get_if<fastgltf::math::fmat4x4>(&node.transform)) {
        glm::mat4 m;
        memcpy(&m, mat->data(), sizeof(glm::mat4));
        transform.setFromMatrix(m);
    }

    nodeMap[nodeIndex] = currentObject.get();

    for (size_t childIdx : node.children) {
        loadNode(path, childIdx, asset, scene, currentObject.get(), nodeMap);
    }

    GameObject* currObj = currentObject.get();
    scene->addGameObject(std::move(currentObject));
    return currObj;
}
