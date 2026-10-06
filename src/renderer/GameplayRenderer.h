#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <string>

class SpriteRenderer;
class TextRenderer;
class Texture2D;
struct GameWorld;
class Buildpanel;
class PlacementUI;
class PathVisualizer;
class StatsPanel;
class TowerMenuUI;
class Tower;

class GameplayRenderer {
private:
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;

    std::shared_ptr<Texture2D> m_mainAtlas;
    std::shared_ptr<Texture2D> m_enemyAtlas;
    std::shared_ptr<Texture2D> m_transitionsAtlas;
    std::shared_ptr<Texture2D> m_radiusTexture;
    std::shared_ptr<Texture2D> m_particleTexture;
    std::shared_ptr<Texture2D> m_arrowTexture;
    std::shared_ptr<Texture2D> m_uiBaseTexture;
    std::shared_ptr<Texture2D> m_whiteTexture;

public:
    GameplayRenderer(std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer);

    void loadTextures();

    void renderFrame(
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
        float pistonPlacementAngle = 270.0f,
        float timeScale = 1.0f,
        bool isPaused = false
    );
};

