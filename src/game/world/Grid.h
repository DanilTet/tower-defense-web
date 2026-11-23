#pragma once
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <set>
#include <utility>
#include <cstdint>
#include <glm/glm.hpp>

enum class CellType {
	Ground,    // 0 в layout: Можно строить, можно ходить (враги могут тут идти, пока ты не поставишь башню)
	Path,      // 1 в layout: Нельзя строить, можно ходить (чистая дорога)
	Platform,  // 2 в layout: Можно строить, нельзя ходить (например, гора для снайпера)
	Scenery,   // 3 в layout: Нельзя строить, нельзя ходить (вода/скалы)
	Chasm,     // 4 в layout: Шурф / обрыв (нельзя строить, нельзя ходить)
	Rail,      // 5 в layout: Рельсы / колія (нельзя строить, можно ходить)

	Tower, // башня
	Spawner, // точка старта врагов
	Base, // база игрока
	Empty // пустая ячейка за пределами матрицы
};

class SpriteRenderer;
class Texture2D;

class Grid
{

private:
	int m_width; // количество клеток по ширине
	int m_height; // количество клеток по высоте
	float m_cellSize; // размер одной клетки в пикселях
	glm::vec2 m_offset; // ссув поля

	std::vector<std::vector<CellType>> m_grid; // 2D вектор для хранения типа каждой клетки
	std::vector<std::vector<CellType>> m_originalGrid; // Хранилище исходных типов клеток (до постройки башен)
	std::vector<std::vector<bool>> m_noBuildMask; // [y][x], true = no-build zone
	bool m_hasSidePockets = false;
	std::map<std::pair<int, int>, CellType> m_outerCells;
	std::map<std::pair<int, int>, CellType> m_originalOuterCells;
	std::set<std::pair<int, int>> m_outerNoBuild;
	uint64_t m_version = 0;

	void renderCellVisual(SpriteRenderer* renderer,
		const Texture2D* whiteTexture,
		int x, int y, CellType type,
		glm::vec2 pixelPos, glm::vec3 color,
		bool isMineTheme, bool isCanalTheme) const;

public:
	Grid(int width, int height, float cellSize, glm::vec2 offset = glm::vec2(0.0f, 0.0f));

	void saveOriginalGrid();
	CellType getOriginalCellType(int gridX, int gridY) const;

	glm::vec2 gridToPixel(int gridX, int gridY) const; // тут получаем левый верхний угол клетки
	glm::ivec2 pixelToGrid(glm::vec2 pixelPos) const; // получаем кординаты мыши и возращаем индекс клетки в масиве

	bool canBuildAt(int gridX, int gridY) const; // проверяем можно ли построить башню на этой клетке
	void setCellType(int gridX, int gridY, CellType type, bool bumpVersion = true); // устанавливаем тип клетки
	uint64_t getVersion() const { return m_version; }

	// СЛОЙ 1: Базовые клетки матрицы и внешние клетки (Ground, Path, Platform, Chasm, Rail)
	void drawBaseCells(SpriteRenderer* renderer,
		const Texture2D* whiteTexture,
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f),
		const std::string& backgroundPreset = "default") const;

	// СЛОЙ 2: Мягкая сетка редактора поверх фона (линии свободных границ и внешних клеток)
	void drawEditorGridLines(SpriteRenderer* renderer,
		const Texture2D* whiteTexture) const;

	// СЛОЙ 3: Геймплейные препятствия (CellType::Scenery - ящики, стены)
	void drawObstacleCells(SpriteRenderer* renderer,
		const Texture2D* whiteTexture,
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f),
		const std::string& backgroundPreset = "default") const;

	// СЛОЙ 4: Оверлей зон запрета стройки на внешних клетках
	void drawOuterNoBuildOverlay(SpriteRenderer* renderer,
		const Texture2D* whiteTexture) const;

	void draw(SpriteRenderer* renderer,
		const Texture2D* whiteTexture,
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f),
		const std::string& backgroundPreset = "default");

	void updateCellSize(int windowWidth, int windowHeight, float bottomMargin = 0.0f, float topMargin = 0.0f); // обновляем размер клеток с учетом доступной высоты
	float getCellSize() const { return m_cellSize; } // получаем размер клетки
	glm::vec2 getOffset() const { return m_offset; }

	// геттеры для алгоритма А*
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	CellType getCellType(int gridX, int gridY) const;
	void setNoBuildMask(const std::vector<std::vector<bool>>& mask);
	bool isNoBuild(int gridX, int gridY) const;
	bool isNoBuildAt(int gridX, int gridY) const { return isNoBuild(gridX, gridY); }
	void setNoBuildAt(int gridX, int gridY, bool value);
	void removeNoBuildAt(int gridX, int gridY);
	void setHasSidePockets(bool val) { m_hasSidePockets = val; }
	bool hasSidePockets() const { return m_hasSidePockets; }

	const std::map<std::pair<int, int>, CellType>& getOuterCells() const { return m_outerCells; }
	const std::map<std::pair<int, int>, CellType>& getOriginalOuterCells() const { return m_originalOuterCells; }
	const std::set<std::pair<int, int>>& getOuterNoBuild() const { return m_outerNoBuild; }
	void setOuterCells(const std::map<std::pair<int, int>, CellType>& cells) { m_outerCells = cells; }
	void setOuterNoBuild(const std::set<std::pair<int, int>>& noBuild) { m_outerNoBuild = noBuild; }
	void clearOuterCells() { m_outerCells.clear(); m_originalOuterCells.clear(); m_outerNoBuild.clear(); }
};
