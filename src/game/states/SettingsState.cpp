#include "SettingsState.h"
#include "GameStateManager.h"
#include "../core/LocalizationManager.h"
#include "../core/SettingsManager.h"
#include "../ui/UICommon.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../resources/ResourceManager.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

SettingsState::SettingsState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer)
{
    m_whiteTexture = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});
    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});
}

void SettingsState::updateLayout() {
    float scale = GetUIScale(m_width, m_height);
    m_uiScale = scale;

    m_windowSize = glm::vec2(
        std::clamp(520.0f * scale, 380.0f, static_cast<float>(m_width) - 40.0f),
        std::clamp(410.0f * scale, 300.0f, static_cast<float>(m_height) - 40.0f)
    );

    if (!m_isDragging) {
        m_windowPos = glm::vec2(
            (static_cast<float>(m_width) - m_windowSize.x) * 0.5f,
            (static_cast<float>(m_height) - m_windowSize.y) * 0.5f
        );
    } else {
        m_windowPos.x = std::clamp(m_windowPos.x, 0.0f, std::max(0.0f, static_cast<float>(m_width) - m_windowSize.x));
        m_windowPos.y = std::clamp(m_windowPos.y, 0.0f, std::max(0.0f, static_cast<float>(m_height) - m_windowSize.y));
    }

    m_headerHeight = std::clamp(40.0f * scale, 30.0f, 56.0f);

    m_volumeWidget.setScale(scale);
    m_volumeWidget.setWidth(m_windowSize.x - 60.0f * scale);
    m_volumeWidget.setLabel(LOC("SETTINGS_VOLUME"));

    float volH = m_volumeWidget.getHeight();

    float langBtnW = std::clamp(80.0f * scale, 56.0f, 110.0f);
    float langBtnH = std::clamp(34.0f * scale, 24.0f, 48.0f);
    m_langBtnSize = glm::vec2(langBtnW, langBtnH);

    m_closeBtnSize = glm::vec2(std::clamp(180.0f * scale, 120.0f, 240.0f), std::clamp(40.0f * scale, 28.0f, 52.0f));

    m_volRelY = m_headerHeight + std::clamp(18.0f * scale, 12.0f, 24.0f);
    m_langRelY = m_volRelY + volH + std::clamp(16.0f * scale, 10.0f, 22.0f);
    m_scaleLabelRelY = m_langRelY + langBtnH + std::clamp(14.0f * scale, 8.0f, 20.0f);
    m_scaleBtnsRelY = m_scaleLabelRelY + std::clamp(24.0f * scale, 16.0f, 32.0f);
    m_closeRelY = m_windowSize.y - m_closeBtnSize.y - std::clamp(20.0f * scale, 14.0f, 28.0f);

    updatePositions();
}

void SettingsState::updatePositions() {
    float scale = m_uiScale;
    float padX = 30.0f * scale;

    m_volumeWidget.setPosition(m_windowPos + glm::vec2(padX, m_volRelY));

    float langGap = std::clamp(10.0f * scale, 6.0f, 16.0f);
    float totalLangW = 3.0f * m_langBtnSize.x + 2.0f * langGap;
    float langStartX = m_windowPos.x + m_windowSize.x - padX - totalLangW;

    m_langBtnRuPos = glm::vec2(langStartX, m_windowPos.y + m_langRelY);
    m_langBtnUaPos = glm::vec2(langStartX + m_langBtnSize.x + langGap, m_windowPos.y + m_langRelY);
    m_langBtnEnPos = glm::vec2(langStartX + 2.0f * (m_langBtnSize.x + langGap), m_windowPos.y + m_langRelY);

    m_uiScaleBtns.clear();
    std::vector<int> scalePresets = { 50, 75, 100, 125, 150 };
    float scaleBtnW = std::clamp(64.0f * scale, 46.0f, 88.0f);
    float scaleBtnH = std::clamp(32.0f * scale, 24.0f, 44.0f);
    float scaleGap = std::clamp(10.0f * scale, 5.0f, 14.0f);
    float totalScaleW = static_cast<float>(scalePresets.size()) * scaleBtnW + static_cast<float>(scalePresets.size() - 1) * scaleGap;
    float scaleStartX = m_windowPos.x + (m_windowSize.x - totalScaleW) * 0.5f;
    float scaleY = m_windowPos.y + m_scaleBtnsRelY;

    for (size_t i = 0; i < scalePresets.size(); ++i) {
        UIScalePresetBtn btn;
        btn.percent = scalePresets[i];
        btn.pos = glm::vec2(scaleStartX + i * (scaleBtnW + scaleGap), scaleY);
        btn.size = glm::vec2(scaleBtnW, scaleBtnH);
        m_uiScaleBtns.push_back(btn);
    }

    m_closeBtnPos = m_windowPos + glm::vec2((m_windowSize.x - m_closeBtnSize.x) * 0.5f, m_closeRelY);
}

void SettingsState::init() {
    m_isDragging = false;
    m_dragOffset = glm::vec2(0.0f);
    m_mousePressedLastFrame = true; // Защита от клик-сквозняка (td-ui rule 4)
    m_closeBtnState = 0;

    m_volumeWidget = VolumeSliderWidget(glm::vec2(0.0f), 320.0f, true, LOC("SETTINGS_VOLUME"));

    updateLayout();
}

void SettingsState::cleanup() {}

bool SettingsState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
        point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void SettingsState::processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        SettingsManager::save();
        m_stateManager.popState();
        return;
    }

    // Проверяем динамическое изменение масштаба
    float curScale = GetUIScale(m_width, m_height);
    if (std::abs(curScale - m_uiScale) > 0.001f) {
        updateLayout();
    }

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_currentMousePos = glm::vec2(mouseX, mouseY);

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    bool isPressed = (mouseState == GLFW_PRESS);
    bool justPressed = (isPressed && !m_mousePressedLastFrame);

    if (justPressed) {
        m_mousePressedLastFrame = true;

        // 1. Проверяем взаимодействие с виджетом громкости
        if (m_volumeWidget.handleInput(m_currentMousePos, isPressed, justPressed)) {
            return;
        }

        // 2. Кнопки смены языка
        if (isPointInRect(m_currentMousePos, m_langBtnRuPos, m_langBtnSize)) {
            LocalizationManager::setLanguageByCode("ru");
            m_volumeWidget.setLabel(LOC("SETTINGS_VOLUME"));
            updateLayout();
            return;
        }
        if (isPointInRect(m_currentMousePos, m_langBtnUaPos, m_langBtnSize)) {
            LocalizationManager::setLanguageByCode("ua");
            m_volumeWidget.setLabel(LOC("SETTINGS_VOLUME"));
            updateLayout();
            return;
        }
        if (isPointInRect(m_currentMousePos, m_langBtnEnPos, m_langBtnSize)) {
            LocalizationManager::setLanguageByCode("en");
            m_volumeWidget.setLabel(LOC("SETTINGS_VOLUME"));
            updateLayout();
            return;
        }

        // 3. Кнопки выбора масштаба интерфейса (50%, 75%, 100%, 125%, 150%)
        for (const auto& sbtn : m_uiScaleBtns) {
            if (isPointInRect(m_currentMousePos, sbtn.pos, sbtn.size)) {
                SettingsManager::setUIScalePercent(sbtn.percent);
                SettingsManager::save();
                std::cout << "[Settings] UI scale set to " << sbtn.percent << "%" << std::endl;
                updateLayout(); // Немедленно обновляем и масштабируем окно настроек прямо на экране!
                return;
            }
        }

        // 4. Проверяем клик по шапке окна для перетаскивания
        glm::vec2 headerSize(m_windowSize.x, m_headerHeight);
        if (isPointInRect(m_currentMousePos, m_windowPos, headerSize)) {
            m_isDragging = true;
            m_dragOffset = m_currentMousePos - m_windowPos;
            return;
        }

        // 5. Проверяем клик по кнопке Закрыть
        if (isPointInRect(m_currentMousePos, m_closeBtnPos, m_closeBtnSize)) {
            m_closeBtnState = 2;
            return;
        }
    }
    else if (isPressed) {
        // Мышь зажата
        if (m_volumeWidget.handleInput(m_currentMousePos, isPressed, false)) {
            return;
        }
        if (m_isDragging) {
            m_windowPos = m_currentMousePos - m_dragOffset;
            m_windowPos.x = std::clamp(m_windowPos.x, 0.0f, std::max(0.0f, static_cast<float>(m_width) - m_windowSize.x));
            m_windowPos.y = std::clamp(m_windowPos.y, 0.0f, std::max(0.0f, static_cast<float>(m_height) - m_windowSize.y));
            updatePositions();
        }
    }
    else if (mouseState == GLFW_RELEASE) {
        m_volumeWidget.handleInput(m_currentMousePos, false, false);
        m_isDragging = false;

        if (m_closeBtnState == 2 && isPointInRect(m_currentMousePos, m_closeBtnPos, m_closeBtnSize)) {
            SettingsManager::save();
            m_stateManager.popState();
            return;
        }

        m_closeBtnState = isPointInRect(m_currentMousePos, m_closeBtnPos, m_closeBtnSize) ? 1 : 0;
        m_mousePressedLastFrame = false;
    }
}

void SettingsState::update(float dt) {}

void SettingsState::render() {
    if (!m_renderer || !m_textRenderer) return;

    // Проверяем динамическое изменение масштаба
    float curScale = GetUIScale(m_width, m_height);
    if (std::abs(curScale - m_uiScale) > 0.001f) {
        updateLayout();
    }

    m_renderer->beginBatch();

    // 1. Затемнение фона
    if (m_whiteTexture) {
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.70f));

        // 2. Окно настроек
        m_renderer->drawSprite(m_whiteTexture, m_windowPos, m_windowSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.16f));

        // Рамка окна
        glm::vec3 borderColor(0.24f, 0.26f, 0.32f);
        m_renderer->drawSprite(m_whiteTexture, m_windowPos, glm::vec2(m_windowSize.x, 1.0f), 0.0f, borderColor);
        m_renderer->drawSprite(m_whiteTexture, m_windowPos + glm::vec2(0.0f, m_windowSize.y - 1.0f), glm::vec2(m_windowSize.x, 1.0f), 0.0f, borderColor);
        m_renderer->drawSprite(m_whiteTexture, m_windowPos, glm::vec2(1.0f, m_windowSize.y), 0.0f, borderColor);
        m_renderer->drawSprite(m_whiteTexture, m_windowPos + glm::vec2(m_windowSize.x - 1.0f, 0.0f), glm::vec2(1.0f, m_windowSize.y), 0.0f, borderColor);

        // Шапка окна
        m_renderer->drawSprite(m_whiteTexture, m_windowPos, glm::vec2(m_windowSize.x, m_headerHeight), 0.0f, glm::vec3(0.18f, 0.19f, 0.24f));
        m_renderer->drawSprite(m_whiteTexture, m_windowPos + glm::vec2(0.0f, m_headerHeight), glm::vec2(m_windowSize.x, 2.0f), 0.0f, glm::vec3(1.0f, 0.85f, 0.2f));

        // Кнопки языков (RU, UA, EN)
        std::string curLang = SettingsManager::getLanguage();
        auto drawLangBtn = [&](glm::vec2 pos, const std::string& code) {
            bool isActive = (curLang == code);
            bool isHover = isPointInRect(m_currentMousePos, pos, m_langBtnSize);
            glm::vec3 bg = isActive ? glm::vec3(0.18f, 0.35f, 0.50f) : (isHover ? glm::vec3(0.22f, 0.24f, 0.30f) : glm::vec3(0.16f, 0.18f, 0.24f));
            glm::vec3 border = isActive ? glm::vec3(0.35f, 0.85f, 1.0f) : (isHover ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.28f, 0.30f, 0.38f));
            m_renderer->drawSprite(m_whiteTexture, pos, m_langBtnSize, 0.0f, border);
            m_renderer->drawSprite(m_whiteTexture, pos + glm::vec2(2.0f), m_langBtnSize - glm::vec2(4.0f), 0.0f, bg);
        };

        drawLangBtn(m_langBtnRuPos, "ru");
        drawLangBtn(m_langBtnUaPos, "ua");
        drawLangBtn(m_langBtnEnPos, "en");

        // Кнопки масштаба интерфейса
        int curScalePercent = SettingsManager::getUIScalePercent();
        for (const auto& sbtn : m_uiScaleBtns) {
            bool isActive = (curScalePercent == sbtn.percent);
            bool isHover = isPointInRect(m_currentMousePos, sbtn.pos, sbtn.size);
            glm::vec3 bg = isActive ? glm::vec3(0.18f, 0.35f, 0.50f) : (isHover ? glm::vec3(0.22f, 0.24f, 0.30f) : glm::vec3(0.16f, 0.18f, 0.24f));
            glm::vec3 border = isActive ? glm::vec3(0.35f, 0.85f, 1.0f) : (isHover ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.28f, 0.30f, 0.38f));
            m_renderer->drawSprite(m_whiteTexture, sbtn.pos, sbtn.size, 0.0f, border);
            m_renderer->drawSprite(m_whiteTexture, sbtn.pos + glm::vec2(2.0f), sbtn.size - glm::vec2(4.0f), 0.0f, bg);
        }

        // 3. Кнопка Закрыть
        glm::vec3 btnColor(0.20f, 0.22f, 0.28f);
        if (m_closeBtnState == 1) btnColor = glm::vec3(0.28f, 0.32f, 0.42f);
        if (m_closeBtnState == 2) btnColor = glm::vec3(0.14f, 0.16f, 0.20f);

        m_renderer->drawSprite(m_whiteTexture, m_closeBtnPos, m_closeBtnSize, 0.0f, btnColor);
        m_renderer->drawSprite(m_whiteTexture, m_closeBtnPos, glm::vec2(m_closeBtnSize.x, 1.0f), 0.0f, glm::vec3(0.35f, 0.38f, 0.48f));
        m_renderer->drawSprite(m_whiteTexture, m_closeBtnPos + glm::vec2(0.0f, m_closeBtnSize.y - 1.0f), glm::vec2(m_closeBtnSize.x, 1.0f), 0.0f, glm::vec3(0.35f, 0.38f, 0.48f));
        m_renderer->drawSprite(m_whiteTexture, m_closeBtnPos, glm::vec2(1.0f, m_closeBtnSize.y), 0.0f, glm::vec3(0.35f, 0.38f, 0.48f));
        m_renderer->drawSprite(m_whiteTexture, m_closeBtnPos + glm::vec2(m_closeBtnSize.x - 1.0f, 0.0f), glm::vec2(1.0f, m_closeBtnSize.y), 0.0f, glm::vec3(0.35f, 0.38f, 0.48f));
    }

    m_renderer->flush();

    float fTitle = std::clamp(0.65f * m_uiScale, 0.45f, 0.90f);
    float fLabel = std::clamp(0.52f * m_uiScale, 0.38f, 0.72f);
    float fLangBtn = std::clamp(0.50f * m_uiScale, 0.36f, 0.70f);
    float fScaleBtn = std::clamp(0.48f * m_uiScale, 0.35f, 0.68f);
    float fClose = std::clamp(0.54f * m_uiScale, 0.38f, 0.74f);

    // Заголовок окна с вертикальным центрированием по td-ui
    std::string titleStr = LOC("SETTINGS_TITLE");
    float titleWidth = m_textRenderer->CalculateTextWidth(titleStr, fTitle);
    float titleX = m_windowPos.x + (m_windowSize.x - titleWidth) * 0.5f;
    float titleY = m_windowPos.y + (m_headerHeight - fTitle * 28.0f) * 0.5f + 2.0f;
    m_textRenderer->RenderText(titleStr, titleX, titleY, fTitle, glm::vec3(1.0f, 0.85f, 0.2f));

    // Виджет ползунка громкости и кнопки Mute
    m_volumeWidget.render(m_renderer.get(), m_textRenderer, m_whiteTexture);

    // Подпись выбора языка
    float padX = 30.0f * m_uiScale;
    float langLabelY = m_langBtnRuPos.y + (m_langBtnSize.y - fLabel * 28.0f) * 0.5f + 2.0f;
    m_textRenderer->RenderText(LOC("SETTINGS_LANGUAGE"), m_windowPos.x + padX, langLabelY, fLabel, glm::vec3(0.9f, 0.9f, 0.95f));

    // Текст на кнопках языков с вертикальным центрированием
    auto drawBtnCenterText = [&](glm::vec2 pos, const std::string& txt, bool active) {
        float w = m_textRenderer->CalculateTextWidth(txt, fLangBtn);
        float tx = pos.x + (m_langBtnSize.x - w) * 0.5f;
        float ty = pos.y + (m_langBtnSize.y - fLangBtn * 28.0f) * 0.5f + 2.0f;
        glm::vec3 col = active ? glm::vec3(0.35f, 0.95f, 1.0f) : glm::vec3(0.8f, 0.82f, 0.88f);
        m_textRenderer->RenderText(txt, tx, ty, fLangBtn, col);
    };

    std::string curLang = SettingsManager::getLanguage();
    drawBtnCenterText(m_langBtnRuPos, "РУС", curLang == "ru");
    drawBtnCenterText(m_langBtnUaPos, "УКР", curLang == "ua");
    drawBtnCenterText(m_langBtnEnPos, "ENG", curLang == "en");

    // Подпись и кнопки масштаба интерфейса
    float scaleLabelY = m_windowPos.y + m_scaleLabelRelY;
    m_textRenderer->RenderText(LOC("SETTINGS_UI_SCALE"), m_windowPos.x + padX, scaleLabelY, fLabel, glm::vec3(0.9f, 0.9f, 0.95f));

    int curScalePercent = SettingsManager::getUIScalePercent();
    for (const auto& sbtn : m_uiScaleBtns) {
        bool isActive = (curScalePercent == sbtn.percent);
        std::string pStr = std::to_string(sbtn.percent) + "%";
        float w = m_textRenderer->CalculateTextWidth(pStr, fScaleBtn);
        float tx = sbtn.pos.x + (sbtn.size.x - w) * 0.5f;
        float ty = sbtn.pos.y + (sbtn.size.y - fScaleBtn * 28.0f) * 0.5f + 2.0f;
        glm::vec3 col = isActive ? glm::vec3(0.35f, 0.95f, 1.0f) : glm::vec3(0.8f, 0.82f, 0.88f);
        m_textRenderer->RenderText(pStr, tx, ty, fScaleBtn, col);
    }

    // Текст на кнопке Закрыть с вертикальным центрированием
    std::string closeStr = LOC("SETTINGS_CLOSE");
    float btnTextWidth = m_textRenderer->CalculateTextWidth(closeStr, fClose);
    float closeTextX = m_closeBtnPos.x + (m_closeBtnSize.x - btnTextWidth) * 0.5f;
    float closeTextY = m_closeBtnPos.y + (m_closeBtnSize.y - fClose * 28.0f) * 0.5f + 2.0f;
    m_textRenderer->RenderText(closeStr, closeTextX, closeTextY, fClose, glm::vec3(0.95f, 0.95f, 0.98f));

    m_renderer->endBatch();
}

void SettingsState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    updateLayout();
}
