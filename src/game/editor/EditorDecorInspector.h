#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "../core/LevelManager.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;

struct DecorInspectorLayout {
    glm::vec2 panelPos{0.0f};
    glm::vec2 panelSize{0.0f};
    float headerH = 32.0f;
    glm::vec2 btnCloseCrossPos{0.0f};
    glm::vec2 btnCloseCrossSize{0.0f};

    // Тип декора: [<] [ Название ] [>]
    glm::vec2 btnTypePrevPos{0.0f};
    glm::vec2 btnTypePrevSize{0.0f};
    glm::vec2 btnTypePos{0.0f};
    glm::vec2 btnTypeSize{0.0f};
    glm::vec2 btnTypeNextPos{0.0f};
    glm::vec2 btnTypeNextSize{0.0f};

    // Слой (zIndex): [Слой -1] [ СЛОЙ: N ] [Слой +1]
    glm::vec2 btnLayerDecPos{0.0f};
    glm::vec2 btnLayerDecSize{0.0f};
    glm::vec2 layerValuePos{0.0f};
    glm::vec2 layerValueSize{0.0f};
    glm::vec2 btnLayerIncPos{0.0f};
    glm::vec2 btnLayerIncSize{0.0f};

    // Общий размер: [-] [ 1.0x ] [+]
    glm::vec2 btnScaleDecPos{0.0f};
    glm::vec2 btnScaleDecSize{0.0f};
    glm::vec2 scaleValuePos{0.0f};
    glm::vec2 btnScaleIncPos{0.0f};
    glm::vec2 btnScaleIncSize{0.0f};

    // Масштаб по осям X / Y (доступен для любого декора):
    glm::vec2 btnScaleXDecPos{0.0f};
    glm::vec2 btnScaleXDecSize{0.0f};
    glm::vec2 scaleXValuePos{0.0f};
    glm::vec2 btnScaleXIncPos{0.0f};
    glm::vec2 btnScaleXIncSize{0.0f};

    glm::vec2 btnScaleYDecPos{0.0f};
    glm::vec2 btnScaleYDecSize{0.0f};
    glm::vec2 scaleYValuePos{0.0f};
    glm::vec2 btnScaleYIncPos{0.0f};
    glm::vec2 btnScaleYIncSize{0.0f};

    // Опции прозрачности (puddle / fog):
    bool isPuddle = false;
    glm::vec2 btnOpacityDecPos{0.0f};
    glm::vec2 btnOpacityDecSize{0.0f};
    glm::vec2 opacityValuePos{0.0f};
    glm::vec2 btnOpacityIncPos{0.0f};
    glm::vec2 btnOpacityIncSize{0.0f};

    // Угол поворота: [-45°] [ 0° ] [+45°]
    glm::vec2 btnAngleDecPos{0.0f};
    glm::vec2 btnAngleDecSize{0.0f};
    glm::vec2 btnAnglePos{0.0f};
    glm::vec2 btnAngleSize{0.0f};
    glm::vec2 btnAngleIncPos{0.0f};
    glm::vec2 btnAngleIncSize{0.0f};

    // Сносится башней: [ СНОСИТСЯ: ДА / НЕТ ]
    glm::vec2 btnDestructiblePos{0.0f};
    glm::vec2 btnDestructibleSize{0.0f};

    // Нижние кнопки: [ УДАЛИТЬ ] и [ ЗАКРЫТЬ ]
    glm::vec2 btnDeletePos{0.0f};
    glm::vec2 btnDeleteSize{0.0f};
    glm::vec2 btnClosePos{0.0f};
    glm::vec2 btnCloseSize{0.0f};

    float fontScale = 0.40f;
};

class EditorDecorInspector {
public:
    EditorDecorInspector() = default;

    void resetPosition() {
        m_panelPos = glm::vec2(-1.0f);
        m_customPosition = glm::vec2(0.0f);
        m_hasCustomPosition = false;
        m_lastDecorIdx = -1;
        m_isDragging = false;
    }
    void setPosition(glm::vec2 pos) {
        m_panelPos = pos;
        m_customPosition = pos;
        m_hasCustomPosition = true;
    }
    glm::vec2 getPosition() const { return m_panelPos; }
    bool hasCustomPosition() const { return m_hasCustomPosition; }
    bool isDragging() const { return m_isDragging; }

    DecorInspectorLayout getLayout(const DecorationConfig& config, int decorIndex,
                                   glm::vec2 decorScreenPos,
                                   float screenW, float screenH,
                                   float topBarH, float bottomDockH) const;

    bool handleInput(glm::vec2 mousePos, bool leftDown, bool wasLeftDown,
                     DecorationConfig& config, int decorIndex, glm::vec2 decorScreenPos,
                     float screenW, float screenH, float topBarH, float bottomDockH,
                     const std::function<void()>& onChanged,
                     const std::function<void()>& onDelete,
                     const std::function<void()>& onClose,
                     const std::function<void(const std::string&, const glm::vec4&)>& showToast = nullptr);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const Texture2D* whiteTexture,
                const DecorationConfig& config, int decorIndex, glm::vec2 decorScreenPos,
                float screenW, float screenH, float topBarH, float bottomDockH,
                glm::vec2 mousePos);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const DecorationConfig& config, int decorIndex, glm::vec2 decorScreenPos,
                float screenW, float screenH, float topBarH, float bottomDockH,
                glm::vec2 mousePos);

private:
    static bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);
    static glm::vec2 clampPanelPosition(glm::vec2 pos, glm::vec2 panelSize,
                                        float screenW, float screenH,
                                        float topBarH, float bottomDockH, float uiScale);

    mutable glm::vec2 m_panelPos{-1.0f, -1.0f};
    glm::vec2 m_customPosition{0.0f, 0.0f};
    bool m_hasCustomPosition = false;
    mutable int m_lastDecorIdx = -1;
    bool m_isDragging = false;
    glm::vec2 m_dragOffset{0.0f, 0.0f};
};

