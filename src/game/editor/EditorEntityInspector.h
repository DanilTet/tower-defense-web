#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <functional>
#include "../core/LevelManager.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;

enum class EditorEntityType {
    Spawner,
    Base
};

struct EntityInspectorLayout {
    glm::vec2 panelPos{0.0f};
    glm::vec2 panelSize{0.0f};
    float headerH = 32.0f;
    glm::vec2 btnCloseCrossPos{0.0f};
    glm::vec2 btnCloseCrossSize{0.0f};

    // Ряд 1: ID сущности [-] [ ID: #0 ] [+]
    glm::vec2 btnIdDecPos{0.0f};
    glm::vec2 btnIdDecSize{0.0f};
    glm::vec2 idValuePos{0.0f};
    glm::vec2 idValueSize{0.0f};
    glm::vec2 btnIdIncPos{0.0f};
    glm::vec2 btnIdIncSize{0.0f};

    // Ряд 2: Видимость в бою [ В игре: СКРЫТ / ВИДИМ ]
    glm::vec2 btnVisibilityPos{0.0f};
    glm::vec2 btnVisibilitySize{0.0f};

    // Ряд 3: Плавный спавн / растворение [ Плавный спавн: ВКЛ / ВЫКЛ ]
    glm::vec2 btnFadePos{0.0f};
    glm::vec2 btnFadeSize{0.0f};

    // Ряд 4: Кнопки [ Удалить объект ] и [ Закрыть ]
    glm::vec2 btnDeletePos{0.0f};
    glm::vec2 btnDeleteSize{0.0f};
    glm::vec2 btnClosePos{0.0f};
    glm::vec2 btnCloseSize{0.0f};

    float fontScale = 0.40f;
};

class EditorEntityInspector {
public:
    EditorEntityInspector() = default;

    void resetPosition() { m_panelPos = glm::vec2(-1.0f); m_lastEntityIdx = -1; }
    void setPosition(glm::vec2 pos) { m_panelPos = pos; }
    glm::vec2 getPosition() const { return m_panelPos; }
    bool isDragging() const { return m_isDragging; }

    EntityInspectorLayout getLayout(EditorEntityType type, int entityIndex,
                                    glm::vec2 entityScreenPos,
                                    float screenW, float screenH,
                                    float topBarH, float bottomDockH) const;

    bool handleInput(glm::vec2 mousePos, bool leftDown, bool wasLeftDown,
                     SpawnerData& spawner, int entityIndex, glm::vec2 entityScreenPos,
                     float screenW, float screenH, float topBarH, float bottomDockH,
                     const std::function<void()>& onChanged,
                     const std::function<void()>& onDelete,
                     const std::function<void()>& onClose,
                     const std::function<void(const std::string&, const glm::vec4&)>& showToast = nullptr);

    bool handleInput(glm::vec2 mousePos, bool leftDown, bool wasLeftDown,
                     BaseData& base, int entityIndex, glm::vec2 entityScreenPos,
                     float screenW, float screenH, float topBarH, float bottomDockH,
                     const std::function<void()>& onChanged,
                     const std::function<void()>& onDelete,
                     const std::function<void()>& onClose,
                     const std::function<void(const std::string&, const glm::vec4&)>& showToast = nullptr);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const Texture2D* whiteTexture,
                const SpawnerData& spawner, int entityIndex, glm::vec2 entityScreenPos,
                float screenW, float screenH, float topBarH, float bottomDockH,
                glm::vec2 mousePos);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const SpawnerData& spawner, int entityIndex, glm::vec2 entityScreenPos,
                float screenW, float screenH, float topBarH, float bottomDockH,
                glm::vec2 mousePos);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const Texture2D* whiteTexture,
                const BaseData& base, int entityIndex, glm::vec2 entityScreenPos,
                float screenW, float screenH, float topBarH, float bottomDockH,
                glm::vec2 mousePos);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const BaseData& base, int entityIndex, glm::vec2 entityScreenPos,
                float screenW, float screenH, float topBarH, float bottomDockH,
                glm::vec2 mousePos);

private:
    static bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);

    mutable glm::vec2 m_panelPos{-1.0f, -1.0f};
    mutable int m_lastEntityIdx = -1;
    mutable EditorEntityType m_lastEntityType = EditorEntityType::Spawner;
    bool m_isDragging = false;
    glm::vec2 m_dragOffset{0.0f, 0.0f};
};

