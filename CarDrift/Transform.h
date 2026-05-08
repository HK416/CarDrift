#pragma once

class Transform {
private:
    glm::vec3 m_position{0.0f};
    glm::vec3 m_rotation{0.0f}; // 오일러 각도(Degree)
    glm::vec3 m_scale{1.0f};

    mutable glm::mat4 m_localMatrix{1.0f};
    mutable bool m_dirty = true;

public:
    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::vec3& rot);
    void setScale(const glm::vec3& scale);

    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getRotation() const { return m_rotation; }
    const glm::vec3& getScale() const { return m_scale; }

    const glm::mat4& getLocalMatrix() const;

    bool isDirty() const { return m_dirty; }
    void setDirty() { m_dirty = true; }
};
