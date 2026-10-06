#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <algorithm>

class SpriteRenderer;
class TextRenderer;
class Texture2D;

class VolumeSliderWidget {
private:
    glm::vec2 m_pos{ 0.0f, 0.0f };
    float m_width = 320.0f;
    float m_height = 36.0f;
    float m_scale = 1.0f;
    bool m_isDragging = false;
    bool m_showLabel = true;
    std::string m_label = "Громкость";

public:
    VolumeSliderWidget() = default;
    VolumeSliderWidget(glm::vec2 pos, float width = 320.0f, bool showLabel = true, const std::string& label = "Громкость");

    void setPosition(glm::vec2 pos) { m_pos = pos; }
    void setWidth(float width) { m_width = width; }
    void setScale(float scale) { m_scale = std::clamp(scale, 0.5f, 2.5f); }

    void setLabel(const std::string& label) { m_label = label; }
    const std::string& getLabel() const { return m_label; }
    glm::vec2 getPosition() const { return m_pos; }
    float getWidth() const { return m_width; }
    float getScale() const { return m_scale; }
    float getHeight() const { return (m_showLabel ? (30.0f + 22.0f) : 30.0f) * m_scale; }

    // Обработка пользовательского ввода мыши
    // Возвращает true, если клик или перетаскивание пришлись на этот виджет
    bool handleInput(glm::vec2 mousePos, bool mousePressed, bool mouseJustPressed);

    // Отрисовка виджета
    void render(SpriteRenderer* renderer, TextRenderer* textRenderer, std::shared_ptr<Texture2D> whiteTexture);
};

