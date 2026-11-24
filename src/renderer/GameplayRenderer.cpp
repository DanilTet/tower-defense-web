#include "GameplayRenderer.h"
#include "SpriteRenderer.h"
#include "TextRenderer.h"
#include "BackgroundRenderer.h"
#include "DecorationRenderer.h"
#include "../textures/Texture2D.h"
#include "../resources/ResourceManager.h"
#include "../game/core/ConfigManager.h"
#include "../game/core/LocalizationManager.h"
#include "../game/world/GameWorld.h"
#include "../game/ui/BuildPanel.h"
#include "../game/ui/PlacementUI.h"
#include "../game/ui/PathRenderer.h"
#include "../game/ui/StatsPanel.h"
#include "../game/ui/TowerMenuUI.h"
#include "../game/ui/UICommon.h"
#include "../game/entities/Tower.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>

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
    ResourceManager::loadTexture("classic_tower", "res/textures/towers/classic_tower.png", GL_NEAREST);
    ResourceManager::loadTexture("grassTexture", "res/textures/spr_grass_02.png");
    ResourceManager::loadTexture("radiusTexture", "res/textures/radius2.png");
    ResourceManager::loadTexture("arrowTexture", "res/textures/pathArrow.png");
    ResourceManager::loadTexture("particleTexture", "res/textures/particle.png");
    ResourceManager::loadTexture("uiBaseTexture", "res/textures/ui_space.png");
    ResourceManager::loadTexture("mercury_drop", "res/textures/enemies/mercury_drop.png", GL_NEAREST);
    ResourceManager::loadTexture("enemy_coal", "res/textures/enemies/coal.png");
    ResourceManager::loadTexture("enemy_coal_glow", "res/textures/enemies/coal_glow.png");
    ResourceManager::loadTexture("enemy_spark", "res/textures/enemies/spark.png");

    auto wrapNoDelete = [](const std::string& name) {
        Texture2D* tex = ResourceManager::getTexture(name);
        return tex;
    };

    m_mainAtlas = wrapNoDelete("mainAtlas");
    m_enemyAtlas = wrapNoDelete("enemyAtlas");
    m_transitionsAtlas = wrapNoDelete("transitionsAtlas");
    m_radiusTexture = wrapNoDelete("radiusTexture");
    m_particleTexture = wrapNoDelete("particleTexture");
    m_arrowTexture = wrapNoDelete("arrowTexture");
    m_uiBaseTexture = wrapNoDelete("uiBaseTexture");
    m_whiteTexture = ResourceManager::getWhiteTexture();
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
    bool isPaused,
    bool showPathArrows)
{
    if (!m_renderer) return;

    glm::mat4 P_screen = glm::ortho(0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f, -1.0f, 1.0f);
    glm::mat4 V = world.getViewMatrix(windowWidth, windowHeight);
    glm::mat4 worldProj = P_screen * V;

    // 1. Устанавливаем матрицу камеры для мировых объектов (с учетом Zoom и Pan)
    m_renderer->setProjection(worldProj);
    if (m_textRenderer) m_textRenderer->updateProjection(worldProj);
    m_renderer->beginBatch();

    BackgroundViewRect bgView;
    bgView.min = world.screenToWorld(glm::vec2(0.0f, 0.0f), windowWidth, windowHeight);
    bgView.max = world.screenToWorld(glm::vec2(static_cast<float>(windowWidth), static_cast<float>(windowHeight)), windowWidth, windowHeight);
    bgView.valid = true;

    std::vector<glm::ivec2> baseCells;
    baseCells.reserve(world.bases.size());
    for (const auto& b : world.bases) {
        baseCells.emplace_back(b.x, b.y);
    }

    // СЛОЙ 1: Подложка под фон (базовая земля / трава) и базовые клетки матрицы / outerCells
    if (m_whiteTexture) {
        BackgroundRenderer::renderSubstrate(
            m_renderer.get(),
            m_whiteTexture,
            world.currentBackground,
            world.grid.get(),
            static_cast<float>(windowWidth),
            static_cast<float>(windowHeight),
            bgView
        );
    }
    if (world.grid && m_whiteTexture) {
        world.grid->drawBaseCells(m_renderer.get(), m_whiteTexture, { 1.0f, 1.0f, 1.0f }, world.currentBackground);
    }

    // СЛОЙ 2: Фоновое окружение (настил, рельсы, здание копра, забор, трубы)
    if (m_whiteTexture) {
        BackgroundRenderer::renderEnvironment(
            m_renderer.get(),
            m_whiteTexture,
            world.currentBackground,
            world.grid.get(),
            static_cast<float>(windowWidth),
            static_cast<float>(windowHeight),
            bgView,
            &baseCells
        );
    }

    // СЛОЙ 3: Геймплейные препятствия (CellType::Scenery - ящики, стены)
    if (world.grid && m_whiteTexture) {
        world.grid->drawObstacleCells(m_renderer.get(), m_whiteTexture, { 1.0f, 1.0f, 1.0f }, world.currentBackground);
    }

    // 2.1. Цветные тайлы/маркеры спавнеров и баз (только если visibleInGame == true)
    if (world.grid && m_whiteTexture) {
        float cellSize = world.grid->getCellSize();
        for (const auto& sp : world.spawners) {
            if (sp.visibleInGame) {
                glm::vec2 pPos = world.grid->gridToPixel(sp.pos.x, sp.pos.y);
                glm::vec3 spColor = glm::vec3(0.95f, 0.85f, 0.20f);
                m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, spColor);
                m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, spColor * 0.85f);
            }
        }
        const bool isMineEntrance = (world.currentBackground == "mine_entrance" || world.currentBackground == "mine");
        if (!isMineEntrance) {
            for (const auto& b : world.bases) {
                if (b.visibleInGame) {
                    glm::vec2 pPos = world.grid->gridToPixel(b.x, b.y);
                    glm::vec3 bCol = glm::vec3(0.25f, 0.70f, 0.85f);
                    m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, bCol);
                    m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, bCol * 0.85f);
                }
            }
        }
    }

    // 3. Стрелочки пути
    if (showPathArrows) {
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
    }

    // 3.0. Фоновые эффекты частиц с отрицательным zIndex (ПОД башнями/трубами)
    if (world.grid && m_whiteTexture) {
        world.envParticleManager.render(m_renderer.get(), m_whiteTexture, *world.grid, -100, -1);
    }

    // 3.1. Декорации земли и объектов (ПОД мобами и снарядами, но ПОВЕРХ тайлов)
    if (m_whiteTexture && !world.decorations.empty()) {
        if (world.grid) {
            DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, world.decorations, *world.grid, -1, false);
        } else {
            DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, world.decorations, -1, 64.0f, false);
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

    // 4.15. Верхний слой фона (2.5D забор, козырек копра) — поверх клеток земли и мобов (враги скрываются под сводом)
    if (m_whiteTexture) {
        BackgroundRenderer::renderForeground(
            m_renderer.get(),
            m_whiteTexture,
            world.currentBackground,
            world.grid.get(),
            static_cast<float>(windowWidth),
            static_cast<float>(windowHeight),
            bgView,
            &baseCells
        );
    }

    // 4.2. Меню башни на поле (привязано к мировым координатам башни)
    if (world.grid && selectedTowerOnMap) {
        towerMenuUI.render(selectedTowerOnMap, m_renderer.get(), m_textRenderer, m_uiBaseTexture, *world.grid, world.levelData.getMaxTierForTower(selectedTowerOnMap->getType()));
    }

    // 4.3. Голограмма размещения башни (привязана к мировым координатам курсора)
    glm::vec2 worldMouse = world.screenToWorld(mousePos, windowWidth, windowHeight);
    bool hasPath = !world.paths.empty();
    glm::vec2 currentPanelPos = buildPanel.getUIPanelPos(windowWidth, windowHeight);

    bool isMouseOverHUD = TimeControlUI::getPanelRect(windowWidth, windowHeight).contains(mousePos.x, mousePos.y);
    if (world.grid && !isMouseOverHUD) {
        PlacementPathContext pathContext;
        pathContext.pathfinder = world.pathfinder.get();
        pathContext.spawners = &world.spawners;
        pathContext.bases = &world.bases;
        pathContext.currentPaths = &world.paths;
        placementUI.renderHologram(
            m_renderer.get(),
            m_mainAtlas,
            m_radiusTexture,
            *world.grid,
            worldMouse,
            selectedTowerType,
            world.playerStats,
            currentPanelPos,
            hasPath,
            world.entityManager ? &world.entityManager->getEnemies() : nullptr,
            pistonPlacementAngle,
            &pathContext
        );
    }

    // 4.4. Декорации верхнего слоя (туман/тьма) — ПОВЕРХ мобов, HP-баров, спецэффектов, но ПОД интерфейсом
    if (m_whiteTexture && !world.decorations.empty()) {
        if (world.grid) {
            DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, world.decorations, *world.grid, -1, true);
        } else {
            DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, world.decorations, -1, 64.0f, true);
        }
    }

    // Завершаем пакет отрисовки мировых объектов
    m_renderer->endBatch();

    // 5. Переключаемся на экранную проекцию для UI (HUD, панель строительства, статы)
    m_renderer->setProjection(P_screen);
    if (m_textRenderer) m_textRenderer->updateProjection(P_screen);
    m_renderer->beginBatch();



    statsPanel.drawStatsPanel(world.playerStats, world.waveManager.get(), m_textRenderer, windowWidth, windowHeight);

    buildPanel.BuildRenderUI(
        world.playerStats,
        m_renderer.get(),
        m_textRenderer,
        m_mainAtlas,
        m_uiBaseTexture,
        windowWidth,
        windowHeight,
        selectedTowerType,
        world.levelData.allowedTowers
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
        UIRect autoRect  = TimeControlUI::getAutoWaveButtonRect(windowWidth, windowHeight);
        UIRect waveRect  = TimeControlUI::getWaveButtonRect(windowWidth, windowHeight);

        UIRect hpBadge    = TimeControlUI::getStatsBadgeRect(0, windowWidth, windowHeight);
        UIRect moneyBadge = TimeControlUI::getStatsBadgeRect(1, windowWidth, windowHeight);
        UIRect waveBadge  = TimeControlUI::getStatsBadgeRect(2, windowWidth, windowHeight);

        bool pauseHov = pauseRect.contains(mousePos.x, mousePos.y);
        bool s1Hov    = s1Rect.contains(mousePos.x, mousePos.y);
        bool s2Hov    = s2Rect.contains(mousePos.x, mousePos.y);
        bool s4Hov    = s4Rect.contains(mousePos.x, mousePos.y);
        bool autoHov  = autoRect.contains(mousePos.x, mousePos.y);
        bool waveHov  = waveRect.contains(mousePos.x, mousePos.y);

        bool is1x = (!isPaused && timeScale < 1.5f);
        bool is2x = (!isPaused && timeScale >= 1.5f && timeScale < 3.0f);
        bool is4x = (!isPaused && timeScale >= 3.0f);

        bool isAutoWave = false;
        if (world.waveManager) {
            isAutoWave = world.waveManager->isAutoStartWaves();
        }

        bool canCallWave = world.waveManager ? world.waveManager->canCallEarlyWave() : false;
        bool isBeforeStart = world.waveManager ? (!world.waveManager->isGameStarted() && !world.waveManager->isWaveActive()) : false;
        bool isAllWavesDone = world.waveManager ? world.waveManager->isAllWavesCompleted() : false;
        int bonus = world.waveManager ? world.waveManager->getEarlyCallBonus() : 0;
        float waveCountdown = 0.0f;
        if (world.waveManager) {
            waveCountdown = world.waveManager->getEffectiveWaveCountdown();
        }
        int countdownSec = static_cast<int>(std::ceil(waveCountdown));

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

        // Автопродолжение волн (Авто / AUTO) — яркая зелёная подсветка при включении, приглушённая при выключении
        if (isAutoWave) {
            glm::vec3 autoActiveBg = glm::vec3(0.10f, 0.32f, 0.16f);
            if (autoHov) {
                autoActiveBg = glm::vec3(0.14f, 0.38f, 0.20f);
            }
            drawBtn(autoRect, true, autoHov,
                autoActiveBg,
                glm::vec3(0.32f, 0.96f, 0.52f),
                glm::vec3(0.14f, 0.15f, 0.20f));
        } else {
            drawBtn(autoRect, false, autoHov,
                glm::vec3(0.10f, 0.32f, 0.16f),
                glm::vec3(0.32f, 0.96f, 0.52f),
                glm::vec3(0.14f, 0.15f, 0.20f));
        }

        // Кнопка раннего вызова волны (справа от кнопки Авто под индикатором волны)
        if (isAllWavesDone) {
            // Все волны завершены: неактивна (затемнена)
            drawBtn(waveRect, false, false,
                glm::vec3(0.12f, 0.13f, 0.16f),
                glm::vec3(0.20f, 0.22f, 0.28f),
                glm::vec3(0.12f, 0.13f, 0.16f));
        } else if (isBeforeStart) {
            // До старта игры: сочная зелёная подсветка "Старт [Space]"
            drawBtn(waveRect, true, waveHov,
                glm::vec3(0.12f, 0.32f, 0.18f),
                glm::vec3(0.35f, 0.95f, 0.50f),
                glm::vec3(0.10f, 0.24f, 0.14f));
        } else if (canCallWave) {
            // Во время паузы: акцентный жёлто-зелёный цвет (+N$ бонус)
            drawBtn(waveRect, true, waveHov,
                glm::vec3(0.32f, 0.30f, 0.08f),
                glm::vec3(0.96f, 0.88f, 0.22f),
                glm::vec3(0.22f, 0.20f, 0.08f));
        } else {
            // Во время активного спавна: неактивна (затемнена)
            drawBtn(waveRect, false, false,
                glm::vec3(0.12f, 0.13f, 0.16f),
                glm::vec3(0.20f, 0.22f, 0.28f),
                glm::vec3(0.12f, 0.13f, 0.16f));
        }

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

        // --- Текст кнопки автопродолжения волн ---
        std::string autoBtnText = LOC("HUD_AUTO_WAVE");
        glm::vec3 autoBtnTextColor;
        if (isAutoWave) {
            if (autoHov) {
                autoBtnTextColor = glm::vec3(0.88f, 1.00f, 0.92f);
            } else {
                autoBtnTextColor = glm::vec3(0.55f, 1.00f, 0.68f);
            }
        } else {
            if (autoHov) {
                autoBtnTextColor = glm::vec3(0.78f, 0.82f, 0.90f);
            } else {
                autoBtnTextColor = glm::vec3(0.50f, 0.54f, 0.64f);
            }
        }

        float autoFs = std::clamp(0.40f * scale, 0.28f, 0.54f);
        float autoTw = m_textRenderer->CalculateTextWidth(autoBtnText, autoFs);
        if (autoTw > autoRect.width - 6.0f * scale) {
            autoFs = autoFs * ((autoRect.width - 6.0f * scale) / autoTw);
            autoTw = m_textRenderer->CalculateTextWidth(autoBtnText, autoFs);
        }
        float autoTx = autoRect.x + (autoRect.width - autoTw) * 0.5f;
        float autoTy = autoRect.y + (autoRect.height - autoFs * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(autoBtnText, autoTx, autoTy, autoFs, autoBtnTextColor);

        // --- Текст кнопки вызова волны ---
        std::string waveBtnText;
        glm::vec3 waveBtnTextColor;
        if (isAllWavesDone) {
            waveBtnText = "Победа!";
            waveBtnTextColor = glm::vec3(0.40f, 0.44f, 0.52f);
        } else if (isBeforeStart) {
            waveBtnText = "Старт";
            if (waveHov) {
                waveBtnTextColor = glm::vec3(1.0f, 1.0f, 1.0f);
            } else {
                waveBtnTextColor = glm::vec3(0.60f, 1.00f, 0.70f);
            }
        } else if (canCallWave) {
            bool allCleared = true;
            if (world.waveManager) {
                allCleared = world.waveManager->areAllEnemiesCleared();
            }
            if (!allCleared && countdownSec > 0 && bonus > 0) {
                waveBtnText = "Ранняя +" + std::to_string(bonus) + "$ (" + std::to_string(countdownSec) + "с)";
            } else if (!allCleared && countdownSec > 0) {
                waveBtnText = "Ранняя (" + std::to_string(countdownSec) + "с)";
            } else {
                waveBtnText = "След. волна";
            }
            if (waveHov) {
                waveBtnTextColor = glm::vec3(1.0f, 1.0f, 1.0f);
            } else {
                waveBtnTextColor = glm::vec3(0.98f, 0.92f, 0.40f);
            }
        } else {
            waveBtnText = "Волна идет...";
            waveBtnTextColor = glm::vec3(0.45f, 0.48f, 0.56f);
        }

        auto renderWaveBtnLabel = [&](const std::string& label, const UIRect& rect, glm::vec3 col) {
            float fs = std::clamp(0.40f * scale, 0.28f, 0.54f);
            float tw = m_textRenderer->CalculateTextWidth(label, fs);
            if (tw > rect.width - 8.0f * scale) {
                fs = fs * ((rect.width - 8.0f * scale) / tw);
                tw = m_textRenderer->CalculateTextWidth(label, fs);
            }
            float tx = rect.x + (rect.width - tw) * 0.5f;
            float ty = rect.y + (rect.height - fs * 28.0f) * 0.5f + 2.0f;
            m_textRenderer->RenderText(label, tx, ty, fs, col);
        };
        renderWaveBtnLabel(waveBtnText, waveRect, waveBtnTextColor);

        // --- Баннер паузы (по центру экрана сверху) ---
        if (isPaused) {
            std::string banner = "[ || ]  ПАУЗА  (P — продолжить)";
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
