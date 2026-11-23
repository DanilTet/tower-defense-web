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

struct EmitterInspectorLayout {
    glm::vec2 panelPos{0.0f};
    glm::vec2 panelSize{0.0f};
    float headerH = 32.0f;
    glm::vec2 btnCloseCrossPos{0.0f};
    glm::vec2 btnCloseCrossSize{0.0f};

    // Режим: Залпы / Постоянный
    glm::vec2 btnModePos{0.0f};
    glm::vec2 btnModeSize{0.0f};

    glm::vec2 btnTypePrevPos{0.0f};
    glm::vec2 btnTypePrevSize{0.0f};
    glm::vec2 btnTypePos{0.0f};
    glm::vec2 btnTypeSize{0.0f};
    glm::vec2 btnTypeNextPos{0.0f};
    glm::vec2 btnTypeNextSize{0.0f};

    // Слой (Z-Index): [Слой -1] [ СЛОЙ: N ] [Слой +1]
    glm::vec2 btnLayerDecPos{0.0f};
    glm::vec2 btnLayerDecSize{0.0f};
    glm::vec2 layerValuePos{0.0f};
    glm::vec2 layerValueSize{0.0f};
    glm::vec2 btnLayerIncPos{0.0f};
    glm::vec2 btnLayerIncSize{0.0f};

    glm::vec2 btnAngleDecPos{0.0f};
    glm::vec2 btnAngleDecSize{0.0f};
    glm::vec2 btnAnglePos{0.0f};
    glm::vec2 btnAngleSize{0.0f};
    glm::vec2 btnAngleIncPos{0.0f};
    glm::vec2 btnAngleIncSize{0.0f};

    glm::vec2 btnSpeedDecPos{0.0f};
    glm::vec2 btnSpeedDecSize{0.0f};
    glm::vec2 speedValuePos{0.0f};
    glm::vec2 btnSpeedIncPos{0.0f};
    glm::vec2 btnSpeedIncSize{0.0f};

    glm::vec2 btnScaleDecPos{0.0f};
    glm::vec2 btnScaleDecSize{0.0f};
    glm::vec2 scaleValuePos{0.0f};
    glm::vec2 btnScaleIncPos{0.0f};
    glm::vec2 btnScaleIncSize{0.0f};

    glm::vec2 btnPeriodDecPos{0.0f};
    glm::vec2 btnPeriodDecSize{0.0f};
    glm::vec2 periodValuePos{0.0f};
    glm::vec2 btnPeriodIncPos{0.0f};
    glm::vec2 btnPeriodIncSize{0.0f};

    glm::vec2 btnBurstDecPos{0.0f};
    glm::vec2 btnBurstDecSize{0.0f};
    glm::vec2 burstValuePos{0.0f};
    glm::vec2 btnBurstIncPos{0.0f};
    glm::vec2 btnBurstIncSize{0.0f};

    glm::vec2 btnDeletePos{0.0f};
    glm::vec2 btnDeleteSize{0.0f};
    glm::vec2 btnClosePos{0.0f};
    glm::vec2 btnCloseSize{0.0f};

    float fontScale = 0.40f;
};

class EditorEmitterInspector {
public:
    EditorEmitterInspector() = default;

    void resetPosition() { m_panelPos = glm::vec2(-1.0f); m_lastEmitterIdx = -1; }
    void setPosition(glm::vec2 pos) { m_panelPos = pos; }
    glm::vec2 getPosition() const { return m_panelPos; }
    bool isDragging() const { return m_isDragging; }

    EmitterInspectorLayout getLayout(const ParticleEmitterConfig& config, int emitterIndex,
                                     glm::vec2 emitterScreenPos,
                                     float screenW, float screenH,
                                     float topBarH, float bottomDockH) const;

    bool handleInput(glm::vec2 mousePos, bool leftDown, bool wasLeftDown,
                     ParticleEmitterConfig& config, int emitterIndex, glm::vec2 emitterScreenPos,
                     float screenW, float screenH, float topBarH, float bottomDockH,
                     const std::function<void()>& onChanged,
                     const std::function<void()>& onDelete,
                     const std::function<void()>& onClose,
                     const std::function<void(const std::string&, const glm::vec4&)>& showToast = nullptr);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const Texture2D* whiteTexture,
                const ParticleEmitterConfig& config, int emitterIndex, glm::vec2 emitterScreenPos,
                float screenW, float screenH, float topBarH, float bottomDockH,
                glm::vec2 mousePos);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const ParticleEmitterConfig& config, int emitterIndex, glm::vec2 emitterScreenPos,
                float screenW, float screenH, float topBarH, float bottomDockH,
                glm::vec2 mousePos);

private:
    static bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);

    mutable glm::vec2 m_panelPos{-1.0f, -1.0f};
    mutable int m_lastEmitterIdx = -1;
    bool m_isDragging = false;
    glm::vec2 m_dragOffset{0.0f, 0.0f};
};

