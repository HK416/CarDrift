#include "stdafx.h"
#include "CascadeShadow.h"

CascadeResult CascadeShadow::calculate(
    const glm::mat4& cameraView,
    const glm::mat4& cameraProj,
    const glm::vec3& lightDirection,
    float nearZ,
    float farZ,
    float fovY,
    float aspect,
    uint32_t cascadeCount,
    float splitLambda
) {
    CascadeResult result;
    result.cascadeCount = cascadeCount;
    result.cascadeSplits = glm::vec4(0.0f);

    std::vector<float> cascadeSplits(cascadeCount + 1);
    cascadeSplits[0] = nearZ;
    cascadeSplits[cascadeCount] = farZ;

    for (uint32_t i = 1; i < cascadeCount; ++i) {
        float p = (float)i / (float)cascadeCount;
        float log = nearZ * std::pow(farZ / nearZ, p);
        float uniform = nearZ + (farZ - nearZ) * p;
        cascadeSplits[i] = glm::mix(uniform, log, splitLambda);
    }

    for (uint32_t i = 0; i < std::min(cascadeCount - 1, 4u); ++i) {
        result.cascadeSplits[i] = cascadeSplits[i + 1];
    }

    glm::vec3 ndcCorners[8] = {
        {-1.0f, -1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {-1.0f, -1.0f, 1.0f},
        {1.0f, -1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {-1.0f, 1.0f, 1.0f}
    };

    glm::mat4 invCam = glm::inverse(cameraProj * cameraView);
    glm::vec3 frustumCornerWorld[8];
    for (uint32_t i = 0; i < 8; ++i) {
        glm::vec4 invCorner = invCam * glm::vec4(ndcCorners[i], 1.0f);
        frustumCornerWorld[i] = invCorner / invCorner.w;
    }

    for (uint32_t i = 0; i < cascadeCount; ++i) {
        float splitNear = cascadeSplits[i];
        float splitFar = cascadeSplits[i + 1];

        glm::vec3 cascadeCorners[8];
        for (uint32_t j = 0; j < 4; ++j) {
            glm::vec3 cornerRay = frustumCornerWorld[j + 4] - frustumCornerWorld[j];
            float nearRatio = (splitNear - nearZ) / (farZ - nearZ);
            float farRatio = (splitFar - nearZ) / (farZ - nearZ);

            cascadeCorners[j] = frustumCornerWorld[j] + cornerRay * nearRatio;
            cascadeCorners[j + 4] = frustumCornerWorld[j] + cornerRay * farRatio;
        }

        glm::vec3 frustumCenter = glm::vec3(0.0f);
        for (uint32_t j = 0; j < 8; ++j) {
            frustumCenter += cascadeCorners[j];
        }
        frustumCenter /= 8.0f;

        glm::vec3 lightUp = glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::abs(glm::dot(lightDirection, lightUp)) > 0.99f) {
            lightUp = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        glm::mat4 lightView = glm::lookAt(frustumCenter - lightDirection, frustumCenter, lightUp);

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();

        for (uint32_t j = 0; j < 8; j++) {
            glm::vec4 v = lightView * glm::vec4(cascadeCorners[j], 1.0f);
            minX = std::min(minX, v.x);
            maxX = std::max(maxX, v.x);
            minY = std::min(minY, v.y);
            maxY = std::max(maxY, v.y);
            minZ = std::min(minZ, v.z);
            maxZ = std::max(maxZ, v.z);
        }

        float zMult = 10.0f;
        minZ = minZ < 0.0f ? minZ * zMult : minZ / zMult;
        maxZ = maxZ < 0.0f ? maxZ / zMult : maxZ * zMult;

        glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
        lightProj[1][1] *= -1.0f;

        result.shadowMatrices[i] = lightProj * lightView;
    }

    return result;
}
