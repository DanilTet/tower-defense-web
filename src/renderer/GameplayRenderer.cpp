#include "GameplayRenderer.h"
#include "SpriteRenderer.h"
#include "TextRenderer.h"
#include "../textures/Texture2D.h"
#include "../resources/ResourceManager.h"
#include "../game/core/ConfigManager.h"
#include "../game/world/GameWorld.h"
#include "../game/ui/BuildPanel.h"
#include "../game/ui/PlacementUI.h"
#include "../game/ui/PathRenderer.h"
#include "../game/ui/StatsPanel.h"
#include "../game/ui/TowerMenuUI.h"
#include "../game/ui/UICommon.h"
#include "../game/entities/Tower.h"

static std::string getPistonDirectionName(float angle) {
    if (std::abs(angle - 270.0f) < 5.0f || std::abs(angle - (-90.0f)) < 5.0f) return "ВВЕРХ";
    if (std::abs(angle - 0.0f) < 5.0f) return "ВПРАВО";
    if (std::abs(angle - 90.0f) < 5.0f) return "ВНИЗ";
    if (std::abs(angle - 180.0f) < 5.0f) return "ВЛЕВО";
    return "ВВЕРХ";
}

GameplayRenderer::GameplayRenderer(std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_renderer(renderer), m_textRenderer(textRenderer)
{
    loadTextures();
}

void GameplayRenderer::loadTextures() {
    // Загрузка атласов и текстур в ResourceManager при необходимости
    ResourceManager::loadTexture("mainAtlas", "res/textures/mainAtlas.png");
    ResourceManager::loadTexture("enemyAtlas", "res/textures/enemyAtlas.png");
    ResourceManager::loadTexture("transitionsAtlas", "res/textures/Frame 1 (2).png");
    ResourceManager::loadTexture("towerTexture", "res/textures/test_sprite.png");
    ResourceManager::loadTexture("grassTexture", "res/textures/spr_grass_02.png");
    ResourceManager::loadTexture("radiusTexture", "res/textures/radius2.png");
    ResourceManager::loadTexture("arrowTexture", "res/textures/pathArrow.png");
    ResourceManager::loadTexture("particleTexture", "res/textures/particle.png");
    ResourceManager::loadTexture("uiBaseTexture", "res/textures/ui_space.png");

    auto wrapNoDelete = [](const std::string& name) {
        Texture2D* tex = ResourceManager::getTexture(name);
        return std::shared_ptr<Texture2D>(tex, [](Texture2D*) {});
    };

    m_mainAtlas = wrapNoDelete("mainAtlas");
    m_enemyAtlas = wrapNoDelete("enemyAtlas");
    m_transitionsAtlas = wrapNoDelete("transitionsAtlas");
    m_radiusTexture = wrapNoDelete("radiusTexture");
    m_particleTexture = wrapNoDelete("particleTexture");
    m_arrowTexture = wrapNoDelete("arrowTexture");
    m_uiBaseTexture = wrapNoDelete("uiBaseTexture");
    m_whiteTexture = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});
}

void GameplayRenderer::renderFrame(
    GameWorld& world,
    Buildpanel& buildPanel,
    PlacementUI& placementUI,
    PathVisualizer& pathVisualizer,
    StatsPanel& statsPanel,
    TowerMenuUI& towerMenuUI,
    const std::string& selectedTowerType,
    Tower* selectedTowerOnMap,
    const glm::vec2& mousePos,
    int windowWidth,
    int windowHeight,
    float pistonPlacementAngle,
    float timeScale,
    bool isPaused)
{
    if (!m_renderer) return;

    m_renderer->beginBatch();

    // 1. Отрисовка сплошного минималистичного фона и нижней панели (Dock)
    if (m_whiteTexture) {
        // Основной фон
        m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f, 0.0f), glm::vec2(windowWidth, windowHeight), 0.0f, glm::vec3(0.12f, 0.13f, 0.16f));

        // Полоса нижней панели управления (Dock)
        float bottomBarHeight = Buildpanel::getBottomBarHeight(windowWidth, windowHeight);
        float barY = static_cast<float>(windowHeight) - bottomBarHeight;

        // Разделительная линия сверху нижней панели
        m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f, barY), glm::vec2(static_cast<float>(windowWidth), 2.0f), 0.0f, glm::vec3(0.22f, 0.24f, 0.29f));
        // Темная подложка дока
        m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f, barY + 2.0f), glm::vec2(static_cast<float>(windowWidth), bottomBarHeight - 2.0f), 0.0f, glm::vec3(0.08f, 0.09f, 0.11f));
    }

    // 2. Отрисовка игровой сетки
    if (world.grid) {
        world.grid->draw(m_renderer.get(), m_whiteTexture, { 1.0f, 1.0f, 1.0f });
    }

    // 3. Стрелочки пути
    for (const auto& path : world.paths) {
        if (world.grid) {
            pathVisualizer.renderPathArrows(
                m_renderer.get(),
                m_arrowTexture,
                path,
                *world.grid
            );
        }
    }

    // 4. Отрисовка сущностей (башни, враги, пули, частицы)
    if (world.entityManager && world.grid) {
        world.entityManager->render(
            m_renderer.get(),
            m_mainAtlas,
            m_enemyAtlas,
            m_radiusTexture,
            m_arrowTexture,
            m_particleTexture,
            *world.grid,
            selectedTowerOnMap
        );
    }

    // 4.1. Отрисовка вагонетки и опасности на рельсах
    world.render(m_renderer.get(), m_whiteTexture);

    m_renderer->flush();

    // 5. Отрисовка интерфейса
    if (world.grid) {
        towerMenuUI.render(selectedTowerOnMap, m_renderer.get(), m_textRenderer, m_uiBaseTexture, *world.grid);
    }

    bool hasPath = !world.paths.empty();
    glm::vec2 currentPanelPos = buildPanel.getUIPanelPos(windowWidth, windowHeight);

    if (world.grid) {
        placementUI.renderHologram(
            m_renderer.get(),
            m_mainAtlas,
            m_radiusTexture,
            *world.grid,
            mousePos,
            selectedTowerType,
            world.playerStats,
            currentPanelPos,
            hasPath,
            world.entityManager ? &world.entityManager->getEnemies() : nullptr,
            pistonPlacementAngle
        );
    }

    statsPanel.drawStatsPanel(world.playerStats, world.waveManager.get(), m_textRenderer, windowWidth, windowHeight);

    buildPanel.BuildRenderUI(
        world.playerStats,
        m_renderer.get(),
        m_textRenderer,
        m_mainAtlas,
        m_uiBaseTexture,
        windowWidth,
        windowHeight,
        selectedTowerType
    );

    // 6. Подсказка управления поворотом для поршня (клавиша R)
    if (m_textRenderer && m_whiteTexture) {
        std::string hintText = "";
        if (selectedTowerType == "Piston") {
            hintText = "[R] - Повернуть поршень (Направление: " + getPistonDirectionName(pistonPlacementAngle) + ")";
        }
        else if (selectedTowerOnMap && selectedTowerOnMap->getType() == "Piston") {
            hintText = "[R] - Повернуть поршень (Направление: " + getPistonDirectionName(selectedTowerOnMap->getAngle()) + ")";
        }

        if (!hintText.empty()) {
            float scale = GetUIScale(windowWidth, windowHeight);
            float bottomBarHeight = Buildpanel::getBottomBarHeight(windowWidth, windowHeight);
            float fontScale = 0.52f * scale;
            float textWidth = m_textRenderer->CalculateTextWidth(hintText, fontScale);
            float hintX = (static_cast<float>(windowWidth) - textWidth) * 0.5f;
            float hintY = static_cast<float>(windowHeight) - bottomBarHeight - (28.0f * scale);

            // Подложка бейджа
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(hintX - 16.0f * scale, hintY - 6.0f * scale), glm::vec2(textWidth + 32.0f * scale, 28.0f * scale), 0.0f, glm::vec3(0.08f, 0.09f, 0.12f));
            // Золотистая акцентная черта слева
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(hintX - 16.0f * scale, hintY - 6.0f * scale), glm::vec2(4.0f * scale, 28.0f * scale), 0.0f, glm::vec3(1.0f, 0.85f, 0.2f));
            m_renderer->flush();

            m_textRenderer->RenderText(hintText, hintX, hintY + 1.0f * scale, fontScale, glm::vec3(1.0f, 0.92f, 0.4f));
        }
    }

    // 7. Единый угловой HUD: Статистика (HP, Деньги, Волна) + Управление временем (Пауза, 1x, 2x, 4x)
    if (m_whiteTexture && m_textRenderer) {
        float scale = GetUIScale(windowWidth, windowHeight);

        UIRect panelRect = TimeControlUI::getPanelRect(windowWidth, windowHeight);
        UIRect pauseRect = TimeControlUI::getPauseButtonRect(windowWidth, windowHeight);
        UIRect s1Rect    = TimeControlUI::getSpeed1xButtonRect(windowWidth, windowHeight);
        UIRect s2Rect    = TimeControlUI::getSpeed2xButtonRect(windowWidth, windowHeight);
        UIRect s4Rect    = TimeControlUI::getSpeed4xButtonRect(windowWidth, windowHeight);

        UIRect hpBadge    = TimeControlUI::getStatsBadgeRect(0, windowWidth, windowHeight);
        UIRect moneyBadge = TimeControlUI::getStatsBadgeRect(1, windowWidth, windowHeight);
        UIRect waveBadge  = TimeControlUI::getStatsBadgeRect(2, windowWidth, windowHeight);

        bool pauseHov = pauseRect.contains(mousePos.x, mousePos.y);
        bool s1Hov    = s1Rect.contains(mousePos.x, mousePos.y);
        bool s2Hov    = s2Rect.contains(mousePos.x, mousePos.y);
        bool s4Hov    = s4Rect.contains(mousePos.x, mousePos.y);

        bool is1x = (!isPaused && timeScale < 1.5f);
        bool is2x = (!isPaused && timeScale >= 1.5f && timeScale < 3.0f);
        bool is4x = (!isPaused && timeScale >= 3.0f);

        // --- Фоновая плашка панели (тёмный графит с рамкой и верхним акцентом) ---
        m_renderer->drawSprite(m_whiteTexture,
            glm::vec2(panelRect.x - 1.0f * scale, panelRect.y - 1.0f * scale),
            glm::vec2(panelRect.width + 2.0f * scale, panelRect.height + 2.0f * scale),
            0.0f, glm::vec3(0.24f, 0.28f, 0.38f));
        m_renderer->drawSprite(m_whiteTexture,
            glm::vec2(panelRect.x, panelRect.y),
            glm::vec2(panelRect.width, panelRect.height),
            0.0f, glm::vec3(0.09f, 0.10f, 0.14f));
        // Верхняя акцентная полоска (2px)
        m_renderer->drawSprite(m_whiteTexture,
            glm::vec2(panelRect.x, panelRect.y),
            glm::vec2(panelRect.width, 2.0f * scale),
            0.0f, glm::vec3(0.35f, 0.50f, 0.75f));

        // --- Бейджи статистики (Строка 1) ---
        auto drawBadgeBg = [&](const UIRect& rect, glm::vec3 bg, glm::vec3 border, glm::vec3 accent) {
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(rect.x, rect.y), glm::vec2(rect.width, rect.height), 0.0f, border);
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(rect.x + 1.0f * scale, rect.y + 1.0f * scale),
                                   glm::vec2(rect.width - 2.0f * scale, rect.height - 2.0f * scale), 0.0f, bg);
            // Акцентная полоска слева 2.5px
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(rect.x + 1.0f * scale, rect.y + 1.0f * scale),
                                   glm::vec2(2.5f * scale, rect.height - 2.0f * scale), 0.0f, accent);
        };

        // База HP — кораллово-красный
        drawBadgeBg(hpBadge, glm::vec3(0.16f, 0.10f, 0.12f), glm::vec3(0.42f, 0.20f, 0.24f), glm::vec3(1.0f, 0.35f, 0.40f));
        // Деньги — золотой
        drawBadgeBg(moneyBadge, glm::vec3(0.16f, 0.14f, 0.08f), glm::vec3(0.46f, 0.38f, 0.16f), glm::vec3(1.0f, 0.85f, 0.22f));
        // Волна — мятный изумруд
        drawBadgeBg(waveBadge, glm::vec3(0.09f, 0.15f, 0.12f), glm::vec3(0.18f, 0.42f, 0.28f), glm::vec3(0.35f, 0.95f, 0.55f));

        // --- Разделительная линия между строкой статистики и кнопками ---
        float divY = panelRect.y + (TimeControlUI::PANEL_PAD_Y + TimeControlUI::STATS_ROW_H + 2.0f) * scale;
        m_renderer->drawSprite(m_whiteTexture,
            glm::vec2(panelRect.x + TimeControlUI::PANEL_PAD_X * scale, divY),
            glm::vec2(panelRect.width - TimeControlUI::PANEL_PAD_X * 2.0f * scale, 1.0f * scale),
            0.0f, glm::vec3(0.18f, 0.22f, 0.30f));

        // --- Кнопки управления временем (Строка 2) ---
        auto drawBtn = [&](const UIRect& rect, bool active, bool hov,
                           glm::vec3 activeBg, glm::vec3 activeBorder,
                           glm::vec3 inactiveBg) {
            glm::vec3 bg     = active ? activeBg     : (hov ? glm::vec3(0.22f, 0.26f, 0.35f) : inactiveBg);
            glm::vec3 border = active ? activeBorder : (hov ? glm::vec3(0.55f, 0.60f, 0.72f) : glm::vec3(0.30f, 0.34f, 0.44f));
            float brd = 1.5f * scale;

            m_renderer->drawSprite(m_whiteTexture,
                glm::vec2(rect.x, rect.y), glm::vec2(rect.width, rect.height),
                0.0f, border);
            m_renderer->drawSprite(m_whiteTexture,
                glm::vec2(rect.x + brd, rect.y + brd), glm::vec2(rect.width - brd * 2.0f, rect.height - brd * 2.0f),
                0.0f, bg);
            if (active) {
                m_renderer->drawSprite(m_whiteTexture,
                    glm::vec2(rect.x + brd, rect.y + rect.height - 3.0f * scale),
                    glm::vec2(rect.width - brd * 2.0f, 3.0f * scale),
                    0.0f, activeBorder);
            }
        };

        // Пауза — янтарно-золотая
        drawBtn(pauseRect, isPaused, pauseHov,
            glm::vec3(0.32f, 0.22f, 0.07f),
            glm::vec3(1.00f, 0.78f, 0.18f),
            glm::vec3(0.16f, 0.17f, 0.22f));

        // 1x — изумрудно-зелёная
        drawBtn(s1Rect, is1x, s1Hov,
            glm::vec3(0.10f, 0.28f, 0.16f),
            glm::vec3(0.30f, 0.95f, 0.50f),
            glm::vec3(0.16f, 0.17f, 0.22f));

        // 2x — неоново-голубая
        drawBtn(s2Rect, is2x, s2Hov,
            glm::vec3(0.08f, 0.24f, 0.40f),
            glm::vec3(0.25f, 0.78f, 1.00f),
            glm::vec3(0.16f, 0.17f, 0.22f));

        // 4x — пылающая оранжевая
        drawBtn(s4Rect, is4x, s4Hov,
            glm::vec3(0.38f, 0.17f, 0.06f),
            glm::vec3(1.00f, 0.52f, 0.14f),
            glm::vec3(0.16f, 0.17f, 0.22f));

        m_renderer->flush();

        // --- Текст бейджей статистики ---
        auto renderBadgeLabel = [&](const std::string& label, const UIRect& rect, glm::vec3 col) {
            float fs = std::clamp(0.40f * scale, 0.30f, 0.52f);
            float tw = m_textRenderer->CalculateTextWidth(label, fs);
            if (tw > rect.width - 6.0f * scale) {
                fs = fs * ((rect.width - 6.0f * scale) / tw);
                tw = m_textRenderer->CalculateTextWidth(label, fs);
            }
            float tx = rect.x + (rect.width - tw) * 0.5f + 1.5f * scale;
            float ty = rect.y + (rect.height - fs * 28.0f) * 0.5f + 2.0f;
            m_textRenderer->RenderText(label, tx, ty, fs, col);
        };

        renderBadgeLabel("HP: " + std::to_string(world.playerStats.baseHealth), hpBadge, glm::vec3(1.0f, 0.65f, 0.70f));
        renderBadgeLabel("$ " + std::to_string(world.playerStats.money), moneyBadge, glm::vec3(1.0f, 0.90f, 0.40f));
        int waveNum = world.waveManager ? world.waveManager->getCurrentWaveNumber() : 1;
        renderBadgeLabel("Волна: " + std::to_string(waveNum), waveBadge, glm::vec3(0.60f, 1.0f, 0.75f));

        // --- Текст кнопок управления временем ---
        auto renderBtnLabel = [&](const std::string& label, const UIRect& rect, bool active, glm::vec3 activeCol) {
            float fs = std::clamp(0.46f * scale, 0.35f, 0.62f);
            float tw = m_textRenderer->CalculateTextWidth(label, fs);
            float tx = rect.x + (rect.width - tw) * 0.5f;
            float ty = rect.y + (rect.height - fs * 28.0f) * 0.5f + 2.0f;
            glm::vec3 col = active ? activeCol : (glm::vec3(0.62f, 0.66f, 0.78f));
            m_textRenderer->RenderText(label, tx, ty, fs, col);
        };

        renderBtnLabel(isPaused ? ">" : "||", pauseRect, isPaused,  glm::vec3(1.00f, 0.88f, 0.30f));
        renderBtnLabel("1x",                  s1Rect,   is1x,       glm::vec3(0.45f, 1.00f, 0.60f));
        renderBtnLabel("2x",                  s2Rect,   is2x,       glm::vec3(0.40f, 0.88f, 1.00f));
        renderBtnLabel("4x",                  s4Rect,   is4x,       glm::vec3(1.00f, 0.68f, 0.28f));

        // --- Баннер паузы (по центру экрана сверху) ---
        if (isPaused) {
            std::string banner = "[ || ]  ПАУЗА  (Space / P — продолжить)";
            float bfs = std::clamp(0.55f * scale, 0.40f, 0.72f);
            float bw  = m_textRenderer->CalculateTextWidth(banner, bfs);
            float bx  = (static_cast<float>(windowWidth) - bw) * 0.5f;
            float by  = 16.0f * scale;

            m_renderer->drawSprite(m_whiteTexture,
                glm::vec2(bx - 20.0f * scale, by - 6.0f * scale),
                glm::vec2(bw + 40.0f * scale, 34.0f * scale),
                0.0f, glm::vec3(0.09f, 0.10f, 0.14f));
            m_renderer->drawSprite(m_whiteTexture,
                glm::vec2(bx - 20.0f * scale, by - 6.0f * scale),
                glm::vec2(bw + 40.0f * scale, 2.5f * scale),
                0.0f, glm::vec3(1.0f, 0.78f, 0.18f));
            m_renderer->flush();

            m_textRenderer->RenderText(banner, bx, by + 2.0f * scale, bfs, glm::vec3(1.0f, 0.88f, 0.30f));
        }
    }

    m_renderer->endBatch();
}
