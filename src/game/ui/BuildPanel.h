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
		float width = (UI_OFFSET_X * 2.0f) + ((towerCount - 1) * UI_ICON_PADDING) + UI_ICON_SIZE;
		return (width < 120.0f) ? 120.0f : width;
	}
	// кеш
	std::vector<std::string> m_cachedTowers;
	float m_cachedPanelWidth = 0.0f;

public:

	// ИНТЕРФЕЙС ПАНЕЛИ ВЫБОРА БАШНИ
	static constexpr float UI_PANEL_HEIGHT = 92.0f; // высота панели
	static constexpr float UI_ICON_SIZE = 48.0f; // размер иконки
	static constexpr float UI_ICON_PADDING = 88.0f; // отступ
	static constexpr float UI_OFFSET_X = 18.0f; // Отступ иконок от левого края панели
	static constexpr float UI_OFFSET_Y = 10.0f; // Отступ иконок от верхнего края панели

	static float getBottomBarHeight(int windowWidth, int windowHeight);

	void initPanelData();

	// метод для отрисовки панельки
	void BuildRenderUI(
		const PlayerStats& playerStats, // сколька деняяяк
		SpriteRenderer* renderer, // для рисования спрайта
		TextRenderer* textRenderer, // для рисования цен
		std::shared_ptr<Texture2D> mainAtlas, //текстурка
		std::shared_ptr<Texture2D> uiTexture, // для фона
		int windowWidth, // размері окна
		int windowHeight,
		const std::string& selectedTower // выбраная башня
	);

	// чекаем кликнули по панельки ли не
	bool checkClick(float mouseX, float mouseY, int windowWidth, int windowHeight, std::string& selectedTower);
	bool selectTowerByIndex(size_t index, std::string& selectedTower);
	const std::vector<std::string>& getCachedTowers() const { return m_cachedTowers; }
	
	// методы для панельки
	glm::vec2 getUIPanelPos(int windowWidth, int windowHeight) const; // получить позицию панельки относительно верхнего левого угла панельки
	glm::vec2 getTowerIconPos(int index, int windowWidth, int windowHeight) const; // когда отрисовываем иконки башни в менюшки то зависимо от индекса возращает корды
};