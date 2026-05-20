#pragma once

struct CascadeResult {
    uint32_t cascadeCount;
    glm::vec4 cascadeSplits;
    glm::mat4 shadowMatrices[4];
};

class CascadeShadow {
public:
    static CascadeResult calculate(
        const glm::mat4& cameraView,
        const glm::mat4& cameraProj,
        const glm::vec3& lightDirection,
        float nearZ,
        float farZ,
        float fovY,
        float aspect,
        uint32_t cascadeCount = 4,
        float splitLambda = 0.75f
    );
};
