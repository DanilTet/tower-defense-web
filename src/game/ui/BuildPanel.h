#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include "entities/Tower.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;
struct PlayerStats;

class Buildpanel {
private:
	// метод для расчета ширины панели в зависимости от количества башен
	float calculatePanelWidth(size_t towerCount) const {
		if (towerCount == 0) return 120.0f;
		float width = (DOCK_PADDING * 2.0f) + (towerCount * CARD_WIDTH) + ((towerCount > 1 ? (towerCount - 1) : 0) * DOCK_GAP);
		return (width < 120.0f) ? 120.0f : width;
	}
	// кеш
	std::vector<std::string> m_cachedTowers;
	float m_cachedPanelWidth = 0.0f;

public:

	// ИНТЕРФЕЙС ПАНЕЛИ ВЫБОРА БАШНИ (Floating Island Dock)
	static constexpr float CARD_WIDTH = 76.0f;
	static constexpr float CARD_HEIGHT = 86.0f;
	static constexpr float DOCK_PADDING = 8.0f;
	static constexpr float DOCK_GAP = 8.0f;
	static constexpr float DOCK_BOTTOM_MARGIN = 10.0f;
	static constexpr float UI_PANEL_HEIGHT = CARD_HEIGHT + DOCK_PADDING * 2.0f; // 102.0f
	static constexpr float UI_ICON_SIZE = 44.0f;
	static constexpr float UI_ICON_PADDING = CARD_WIDTH + DOCK_GAP; // 84.0f
	static constexpr float UI_OFFSET_X = DOCK_PADDING;
	static constexpr float UI_OFFSET_Y = DOCK_PADDING;

	static float getBottomBarHeight(int windowWidth, int windowHeight);

	void initPanelData(const std::vector<std::string>& allowedTowers = {});
	void setAllowedTowers(const std::vector<std::string>& allowedTowers);

	// метод для отрисовки панельки
	void render(
		const PlayerStats& playerStats,
		SpriteRenderer* renderer,
		TextRenderer* textRenderer,
		const Texture2D* mainAtlas,
		const Texture2D* uiTexture,
		int windowWidth,
		int windowHeight,
		const std::string& selectedTower,
		const std::vector<std::string>& allowedTowers = {}
	) {
		BuildRenderUI(playerStats, renderer, textRenderer, mainAtlas, uiTexture, windowWidth, windowHeight, selectedTower, allowedTowers);
	}

	void BuildRenderUI(
		const PlayerStats& playerStats, // сколька деняяяк
		SpriteRenderer* renderer, // для рисования спрайта
		TextRenderer* textRenderer, // для рисования цен
		const Texture2D* mainAtlas, //текстурка
		const Texture2D* uiTexture, // для фона
		int windowWidth, // размері окна
		int windowHeight,
		const std::string& selectedTower, // выбраная башня
		const std::vector<std::string>& allowedTowers = {}
	);

	// чекаем кликнули по панельки ли не
	bool checkClick(float mouseX, float mouseY, int windowWidth, int windowHeight, std::string& selectedTower);
	bool selectTowerByIndex(size_t index, std::string& selectedTower);
	const std::vector<std::string>& getCachedTowers() const { return m_cachedTowers; }
	
	// методы для панельки
	glm::vec2 getUIPanelPos(int windowWidth, int windowHeight) const; // получить позицию панельки относительно верхнего левого угла панельки
	glm::vec2 getTowerIconPos(int index, int windowWidth, int windowHeight) const; // когда отрисовываем иконки башни в менюшки то зависимо от индекса возращает корды
};