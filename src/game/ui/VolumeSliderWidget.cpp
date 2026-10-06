#include "VolumeSliderWidget.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include "../../audio/AudioManager.h"
#include <algorithm>

VolumeSliderWidget::VolumeSliderWidget(glm::vec2 pos, float width, bool showLabel, const std::string& label)
    : m_pos(pos), m_width(width), m_showLabel(showLabel), m_label(label)
{
}

bool VolumeSliderWidget::handleInput(glm::vec2 mousePos, bool mousePressed, bool mouseJustPressed) {
    float scale = m_scale;
    float contentY = m_showLabel ? (m_pos.y + 22.0f * scale) : m_pos.y;

    float muteBtnX = m_pos.x;
    float muteBtnY = contentY;
    float muteBtnW = 34.0f * scale;
    float muteBtnH = 30.0f * scale;

    float trackX = m_pos.x + muteBtnW + 10.0f * scale;
    float trackWidth = m_width - (muteBtnW + 10.0f * scale) - 58.0f * scale;
    float trackY = contentY + 2.0f * scale;
    float trackHeight = 26.0f * scale; // Увеличенная область захвата клика для комфорта

    if (mouseJustPressed) {
        // Клик по кнопке звука / Mute
        if (mousePos.x >= muteBtnX && mousePos.x <= muteBtnX + muteBtnW &&
            mousePos.y >= muteBtnY && mousePos.y <= muteBtnY + muteBtnH) {
            AudioManager::toggleMute();
            return true;
        }

        // Клик по слайдеру громкости
        if (trackWidth > 10.0f &&
            mousePos.x >= trackX - 6.0f * scale && mousePos.x <= trackX + trackWidth + 6.0f * scale &&
            mousePos.y >= trackY && mousePos.y <= trackY + trackHeight) {
            m_isDragging = true;
            float ratio = (mousePos.x - trackX) / trackWidth;
            ratio = std::clamp(ratio, 0.0f, 1.0f);
            AudioManager::setMasterVolume(ratio);
            return true;
        }
    }

    if (mousePressed && m_isDragging) {
        if (trackWidth > 10.0f) {
            float ratio = (mousePos.x - trackX) / trackWidth;
            ratio = std::clamp(ratio, 0.0f, 1.0f);
            AudioManager::setMasterVolume(ratio);
        }
        return true;
    }

    if (!mousePressed) {
        m_isDragging = false;
    }

    return m_isDragging;
}

void VolumeSliderWidget::render(SpriteRenderer* renderer, TextRenderer* textRenderer, std::shared_ptr<Texture2D> whiteTexture) {
    if (!renderer || !textRenderer || !whiteTexture) return;

    float scale = m_scale;
    float contentY = m_showLabel ? (m_pos.y + 22.0f * scale) : m_pos.y;
    float curVol = AudioManager::getMasterVolume();
    bool isMuted = AudioManager::isMuted();
    float fontScale = std::clamp(0.44f * scale, 0.30f, 0.70f);

    // 1. Рисуем заголовок (если включен)
    if (m_showLabel) {
        textRenderer->RenderText(m_label, m_pos.x, m_pos.y, fontScale, glm::vec3(0.85f, 0.85f, 0.90f));
    }

    float muteBtnX = m_pos.x;
    float muteBtnY = contentY;
    float muteBtnW = 34.0f * scale;
    float muteBtnH = 30.0f * scale;

    float trackX = m_pos.x + muteBtnW + 10.0f * scale;
    float trackWidth = m_width - (muteBtnW + 10.0f * scale) - 58.0f * scale;
    float trackH = std::max(6.0f * scale, 4.0f);
    float trackY = contentY + (muteBtnH - trackH) * 0.5f;

    // 2. Рисуем подложку кнопки Mute
    glm::vec3 btnBg = isMuted ? glm::vec3(0.24f, 0.12f, 0.14f) : glm::vec3(0.16f, 0.18f, 0.24f);
    renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX, muteBtnY), glm::vec2(muteBtnW, muteBtnH), 0.0f, btnBg);

    // Рамка кнопки Mute
    float borderThick = std::max(1.0f * scale, 1.0f);
    glm::vec3 btnBorder = isMuted ? glm::vec3(0.55f, 0.20f, 0.22f) : glm::vec3(0.28f, 0.32f, 0.42f);
    renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX, muteBtnY), glm::vec2(muteBtnW, borderThick), 0.0f, btnBorder);
    renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX, muteBtnY + muteBtnH - borderThick), glm::vec2(muteBtnW, borderThick), 0.0f, btnBorder);
    renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX, muteBtnY), glm::vec2(borderThick, muteBtnH), 0.0f, btnBorder);
    renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX + muteBtnW - borderThick, muteBtnY), glm::vec2(borderThick, muteBtnH), 0.0f, btnBorder);

    // Процедурная иконка динамика
    glm::vec3 iconColor = isMuted ? glm::vec3(0.80f, 0.30f, 0.30f) : glm::vec3(1.0f, 0.85f, 0.25f);
    // Корпус динамика (прямоугольник)
    renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX + 6.0f * scale, muteBtnY + 10.0f * scale), glm::vec2(5.0f * scale, 10.0f * scale), 0.0f, iconColor);
    // Диффузор (ступенчатый рупор)
    renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX + 11.0f * scale, muteBtnY + 8.0f * scale), glm::vec2(3.0f * scale, 14.0f * scale), 0.0f, iconColor);
    renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX + 14.0f * scale, muteBtnY + 6.0f * scale), glm::vec2(3.0f * scale, 18.0f * scale), 0.0f, iconColor);

    if (!isMuted && curVol > 0.01f) {
        // Звуковая волна 1
        renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX + 19.0f * scale, muteBtnY + 8.0f * scale), glm::vec2(2.0f * scale, 14.0f * scale), 0.0f, iconColor);
        if (curVol > 0.35f) {
            // Звуковая волна 2
            renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX + 23.0f * scale, muteBtnY + 6.0f * scale), glm::vec2(2.0f * scale, 18.0f * scale), 0.0f, iconColor);
        }
    }
    else {
        // Индикатор Mute (красная черточка / крестик)
        renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX + 19.0f * scale, muteBtnY + 14.0f * scale), glm::vec2(8.0f * scale, 2.0f * scale), 0.0f, glm::vec3(0.95f, 0.25f, 0.25f));
        renderer->drawSprite(whiteTexture, glm::vec2(muteBtnX + 22.0f * scale, muteBtnY + 11.0f * scale), glm::vec2(2.0f * scale, 8.0f * scale), 0.0f, glm::vec3(0.95f, 0.25f, 0.25f));
    }

    // 3. Рисуем трек слайдера (фон)
    renderer->drawSprite(whiteTexture, glm::vec2(trackX, trackY), glm::vec2(trackWidth, trackH), 0.0f, glm::vec3(0.14f, 0.15f, 0.19f));
    // Обводка трека
    renderer->drawSprite(whiteTexture, glm::vec2(trackX, trackY), glm::vec2(trackWidth, borderThick), 0.0f, glm::vec3(0.24f, 0.26f, 0.32f));
    renderer->drawSprite(whiteTexture, glm::vec2(trackX, trackY + trackH - borderThick), glm::vec2(trackWidth, borderThick), 0.0f, glm::vec3(0.24f, 0.26f, 0.32f));

    // 4. Заполненная часть слайдера
    float fillW = trackWidth * curVol;
    if (fillW > 0.5f) {
        glm::vec3 fillColor = isMuted ? glm::vec3(0.38f, 0.40f, 0.44f) : glm::vec3(1.0f, 0.82f, 0.20f);
        renderer->drawSprite(whiteTexture, glm::vec2(trackX, trackY), glm::vec2(fillW, trackH), 0.0f, fillColor);
    }

    // 5. Бегунок (Thumb)
    float thumbW = 12.0f * scale;
    float thumbH = 20.0f * scale;
    float thumbX = trackX + fillW - (thumbW * 0.5f);
    float thumbY = trackY - ((thumbH - trackH) * 0.5f);

    glm::vec3 thumbColor = isMuted ? glm::vec3(0.60f, 0.62f, 0.68f) : glm::vec3(0.96f, 0.97f, 1.0f);
    renderer->drawSprite(whiteTexture, glm::vec2(thumbX, thumbY), glm::vec2(thumbW, thumbH), 0.0f, thumbColor);
    // Тонкая темная черточка по центру бегунка
    float centerLineW = std::max(2.0f * scale, 1.0f);
    renderer->drawSprite(whiteTexture, glm::vec2(thumbX + (thumbW - centerLineW) * 0.5f, thumbY + 4.0f * scale), glm::vec2(centerLineW, thumbH - 8.0f * scale), 0.0f, glm::vec3(0.25f, 0.27f, 0.32f));

    renderer->flush();

    // 6. Текстовое отображение процентов (каноническое центрирование по высоте mute кнопки)
    float textY = contentY + (muteBtnH - fontScale * 28.0f) * 0.5f + 1.0f;
    float textX = trackX + trackWidth + 10.0f * scale;
    if (isMuted) {
        textRenderer->RenderText("MUTE", textX, textY, fontScale, glm::vec3(1.0f, 0.30f, 0.30f));
    }
    else {
        int percent = static_cast<int>(curVol * 100.0f + 0.5f);
        textRenderer->RenderText(std::to_string(percent) + "%", textX, textY, fontScale, glm::vec3(0.90f, 0.92f, 0.96f));
    }
}

