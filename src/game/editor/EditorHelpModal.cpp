#include "EditorHelpModal.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include "../ui/UICommon.h"
#include <GLFW/glfw3.h>
#include <algorithm>

static bool isPointInRect(glm::vec2 p, glm::vec2 pos, glm::vec2 size) {
    return p.x >= pos.x && p.x <= pos.x + size.x &&
           p.y >= pos.y && p.y <= pos.y + size.y;
}

bool EditorHelpModal::handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                                  bool leftReleased, bool isEscJustPressed)
{
    if (!m_isOpen) return false;

    if (isEscJustPressed) {
        close();
        return true;
    }

    int winW = 1280, winH = 720;
    if (window) {
        glfwGetWindowSize(window, &winW, &winH);
    }
    float scale = GetUIScale(winW, winH);

    float modalW = std::clamp(680.0f * scale, 300.0f, std::max(300.0f, static_cast<float>(winW) - 40.0f));
    float modalH = std::clamp(540.0f * scale, 300.0f, std::max(300.0f, static_cast<float>(winH) - 40.0f));
    glm::vec2 modalPos((static_cast<float>(winW) - modalW) * 0.5f,
                       (static_cast<float>(winH) - modalH) * 0.5f);

    float headerH = 42.0f * scale;
    glm::vec2 crossPos(modalPos.x + modalW - 38.0f * scale, modalPos.y + 7.0f * scale);
    glm::vec2 crossSize(28.0f * scale, 28.0f * scale);

    float btnW = 140.0f * scale;
    float btnH = 32.0f * scale;
    glm::vec2 btnClosePos(modalPos.x + (modalW - btnW) * 0.5f, modalPos.y + modalH - btnH - 12.0f * scale);
    glm::vec2 btnCloseSize(btnW, btnH);

    if (m_justOpened) {
        if (!leftDown) {
            m_justOpened = false;
        }
        m_wasLeftDown = leftDown;
        return true;
    }

    if (leftReleased || (leftDown && !m_wasLeftDown)) {
        if (isPointInRect(mousePos, crossPos, crossSize) ||
            isPointInRect(mousePos, btnClosePos, btnCloseSize)) {
            close();
            m_wasLeftDown = leftDown;
            return true;
        }

        // Клик мимо модального окна закрывает его
        if (!isPointInRect(mousePos, modalPos, glm::vec2(modalW, modalH))) {
            close();
            m_wasLeftDown = leftDown;
            return true;
        }
    }

    m_wasLeftDown = leftDown;
    // Поглощаем все клики внутри модального окна
    return true;
}

void EditorHelpModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                             const Texture2D* whiteTexture,
                             int screenWidth, int screenHeight)
{
    if (!m_isOpen || !renderer || !whiteTexture) return;

    float sw = static_cast<float>(screenWidth);
    float sh = static_cast<float>(screenHeight);
    float scale = GetUIScale(screenWidth, screenHeight);

    // 1. Затемняющая подложка
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(0.0f), glm::vec2(sw, sh), 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.70f));

    // 2. Размеры и позиция модалки
    float modalW = std::clamp(680.0f * scale, 300.0f, std::max(300.0f, sw - 40.0f));
    float modalH = std::clamp(540.0f * scale, 300.0f, std::max(300.0f, sh - 40.0f));
    glm::vec2 modalPos((sw - modalW) * 0.5f, (sh - modalH) * 0.5f);

    // Рамка и фон карточки
    renderer->drawSprite(whiteTexture, modalPos, glm::vec2(modalW, modalH), 0.0f, glm::vec3(0.35f, 0.50f, 0.75f));
    renderer->drawSprite(whiteTexture, modalPos + glm::vec2(2.0f), glm::vec2(modalW - 4.0f, modalH - 4.0f), 0.0f, glm::vec3(0.11f, 0.13f, 0.17f));

    // 3. Шапка модалки
    float headerH = 42.0f * scale;
    renderer->drawSprite(whiteTexture, modalPos + glm::vec2(2.0f), glm::vec2(modalW - 4.0f, headerH), 0.0f, glm::vec3(0.15f, 0.18f, 0.25f));
    renderer->drawSprite(whiteTexture, glm::vec2(modalPos.x + 2.0f, modalPos.y + headerH + 1.0f), glm::vec2(modalW - 4.0f, 1.5f), 0.0f, glm::vec3(0.28f, 0.38f, 0.52f));

    // Кнопка закрытия [X]
    glm::vec2 crossPos(modalPos.x + modalW - 38.0f * scale, modalPos.y + 7.0f * scale);
    glm::vec2 crossSize(28.0f * scale, 28.0f * scale);
    renderer->drawSprite(whiteTexture, crossPos, crossSize, 0.0f, glm::vec3(0.30f, 0.35f, 0.45f));
    renderer->drawSprite(whiteTexture, crossPos + glm::vec2(1.5f), crossSize - glm::vec2(3.0f), 0.0f, glm::vec3(0.20f, 0.23f, 0.30f));

    // 4. Нижняя кнопка [Закрыть]
    float btnW = 140.0f * scale;
    float btnH = 32.0f * scale;
    glm::vec2 btnClosePos(modalPos.x + (modalW - btnW) * 0.5f, modalPos.y + modalH - btnH - 12.0f * scale);
    glm::vec2 btnCloseSize(btnW, btnH);
    renderer->drawSprite(whiteTexture, btnClosePos, btnCloseSize, 0.0f, glm::vec3(0.40f, 0.60f, 0.85f));
    renderer->drawSprite(whiteTexture, btnClosePos + glm::vec2(1.5f), btnCloseSize - glm::vec2(3.0f), 0.0f, glm::vec3(0.18f, 0.24f, 0.34f));

    renderer->flush();

    // 5. Текст справки
    if (textRenderer) {
        // Заголовок
        float titleScale = 0.58f * scale;
        std::string titleStr = "УПРАВЛЕНИЕ И ГОРЯЧИЕ КЛАВИШИ";
        float tw = textRenderer->CalculateTextWidth(titleStr, titleScale);
        textRenderer->RenderText(titleStr, modalPos.x + 20.0f * scale, modalPos.y + 10.0f * scale, titleScale, glm::vec3(1.0f, 0.85f, 0.25f));

        // Крестик [X]
        textRenderer->RenderText("X", crossPos.x + 8.0f * scale, crossPos.y + 5.0f * scale, 0.48f * scale, glm::vec3(0.85f, 0.90f, 0.95f));

        struct HelpItem {
            std::string key;
            std::string desc;
        };

        struct HelpSection {
            std::string title;
            glm::vec3 titleColor;
            std::vector<HelpItem> items;
        };

        std::vector<HelpSection> sections = {
            {
                "[ Навигация и Камера ]",
                glm::vec3(0.40f, 0.85f, 1.0f),
                {
                    { "Колесико мыши", "Зум (приближение / отдаление)" },
                    { "СКМ или Пробел + ЛКМ", "Панорамирование (сдвиг карты)" },
                    { "F", "Сбросить камеру в центр (1.0x)" },
                    { "C", "Режим настройки стартовой камеры для боя" },
                    { "U", "Предпросмотр боевого HUD (Выкл / 100% / 125% / 150%)" }
                }
            },
            {
                "[ Инструменты и Рисование ]",
                glm::vec3(0.40f, 0.95f, 0.55f),
                {
                    { "1-4 (вкладка 3)", "Свищ пара, Капель, Искры, Дым (0/E - Ластик)" },
                    { "Tab / Ctrl+1, 2, 3", "Палитра (Тайлы / Спец / Партиклы)" },
                    { "R / ПКМ по эмиттеру", "Поворот направления струи на 45°" },
                    { "Shift + ЛКМ", "Прямоугольная заливка областью" },
                    { "Ctrl+Z / Ctrl+Y", "Отмена / Повтор действия (Undo / Redo)" },
                    { "Alt + ЛКМ", "Пипетка (взять тайл с карты)" },
                    { "V", "Скрыть / показать стрелки путей" }
                }
            },
            {
                "[ Файлы и Бой ]",
                glm::vec3(1.0f, 0.75f, 0.35f),
                {
                    { "Ctrl + S / S", "Быстрое сохранение карты" },
                    { "W", "Редактор волн" },
                    { "T / Enter", "Тест карты в бою" },
                    { "ESC", "Выход / Закрыть окно" }
                }
            }
        };

        float curY = modalPos.y + headerH + 12.0f * scale;
        float leftColX = modalPos.x + 22.0f * scale;
        float rightColX = modalPos.x + 215.0f * scale;
        float secScale = 0.48f * scale;
        float textScale = 0.42f * scale;
        float rowStep = 18.0f * scale;

        for (const auto& sec : sections) {
            textRenderer->RenderText(sec.title, leftColX, curY, secScale, sec.titleColor);
            curY += 20.0f * scale;

            for (const auto& item : sec.items) {
                textRenderer->RenderText(item.key, leftColX + 10.0f * scale, curY, textScale, glm::vec3(0.95f, 0.95f, 1.0f));
                textRenderer->RenderText("- " + item.desc, rightColX, curY, textScale, glm::vec3(0.72f, 0.76f, 0.82f));
                curY += rowStep;
            }
            curY += 8.0f * scale;
        }

        // Текст кнопки [Закрыть]
        std::string closeStr = "Закрыть";
        float cw = textRenderer->CalculateTextWidth(closeStr, 0.44f * scale);
        textRenderer->RenderText(closeStr, btnClosePos.x + (btnW - cw) * 0.5f, btnClosePos.y + 6.0f * scale, 0.44f * scale, glm::vec3(1.0f, 1.0f, 1.0f));
    }
}
