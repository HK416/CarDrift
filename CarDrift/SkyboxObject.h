#pragma once
#include "GameObject.h"

class SkyboxObject : public MeshObject {
public:
    SkyboxObject() = delete;
    SkyboxObject(const SkyboxObject&) = delete;
    SkyboxObject& operator=(const SkyboxObject&) = delete;

    SkyboxObject(Mesh* cubeMesh, Material* skyboxMaterial);
    virtual ~SkyboxObject() = default;

    virtual void render(RenderQueue& queue) override {};
};
