#pragma once

struct RenderItem {
    glm::mat4 worldMatrix;
    float sortDistance;
};

class RenderQueue {
public:
    RenderQueue() = default;

private:
    std::vector<RenderItem> m_opaqueItems;
};
