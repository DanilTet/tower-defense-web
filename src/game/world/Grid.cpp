#include "Grid.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include "resources/ResourceManager.h"
#include <algorithm>
#include <cmath>

// конструктор создает пустую сетку заданого размера
Grid::Grid(int width, int height, float cellSize, glm::vec2 offset){
	m_width = width; // Запоминаем количество клеток по горизонтали
	m_height = height; // Запоминаем количество клеток по вертикали
	m_cellSize = cellSize; // Задаем стартовый размер одной клетки в пикселях
	m_offset = offset; // запоминаем ссув
	// Двумерный вектор для хранения типа каждой клетки, изначально все клетки пустые
	m_grid.resize(m_height, std::vector<CellType>(m_width, CellType::Ground));
	m_originalGrid.resize(m_height, std::vector<CellType>(m_width, CellType::Ground));
}

void Grid::saveOriginalGrid() {
	m_originalGrid = m_grid;
}

CellType Grid::getOriginalCellType(int gridX, int gridY) const {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		return m_originalGrid[gridY][gridX];
	}
	return CellType::Ground;
}

// Перевод из индексов клетки в пиксели экрана
glm::vec2 Grid::gridToPixel(int gridX, int gridY) const {
	// умножаем индекс строки и столбца на размер клетки в пикселях
	return glm::vec2(gridX * m_cellSize, gridY * m_cellSize) + m_offset;
}

// Перевод из пикселей экрана в индексы клетки сетки (х,у)
glm::ivec2 Grid::pixelToGrid(glm::vec2 pixelPos) const {
	// Считаем смещение сетки. чтобы получить координаты относительно сетки
	float localX = pixelPos.x - m_offset.x;
	float localY = pixelPos.y - m_offset.y;

	// Делим локальные координаты на размер клетки, чтобы получить индексы строки и столбца
	int gridX = static_cast<int>(localX / m_cellSize);
	int gridY = static_cast<int>(localY / m_cellSize);

	// возвращаем индексы клетки в виде целочисленного вектора
	return glm::ivec2(gridX, gridY);
}

// Проверяем можно ли построить башню на клетке с этими индексами
bool Grid::canBuildAt(int gridX, int gridY) const {
	// если координаты вышли за поле то строить нельзя
	if (gridX < 0 || gridX >= m_width || gridY < 0 || gridY >= m_height) {
		return false;
	}

	CellType type = getCellType(gridX, gridY);

	// строить на обрыве/шурфе или на рельсах нельзя
	if (type == CellType::Chasm || type == CellType::Rail) {
		return false;
	}

	// строить можно
	return (type == CellType::Ground || type == CellType::Platform);
}

// Принудительно меняем тип конкретной ячейки
void Grid::setCellType(int gridX, int gridY, CellType type) {
	// защищаем массив от вылета за границы памяти
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		// В нашем движке первый индекс всегда X (столбец), а второй Y (строка)!
		m_grid[gridY][gridX] = type;
	}
}

// Отрисовка всей карты ячейка за ячейкой (минималистичный плоский стиль с темной окантовкой)
void Grid::draw(SpriteRenderer* renderer, std::shared_ptr<Texture2D> whiteTexture, glm::vec3 color) {
	if (!renderer || !whiteTexture) return;

	glm::vec2 cellSizeVec(m_cellSize, m_cellSize);
	float borderWidth = std::max(1.0f, std::round(m_cellSize * 0.04f));
	glm::vec2 innerOffset(borderWidth, borderWidth);
	glm::vec2 innerSize(m_cellSize - 2.0f * borderWidth, m_cellSize - 2.0f * borderWidth);

	for (int y = 0; y < m_height; ++y) {
		for (int x = 0; x < m_width; ++x) {
			glm::vec2 pixelPos = gridToPixel(x, y);

			CellType type = m_grid[y][x];
			if (type == CellType::Tower) {
				type = m_originalGrid[y][x];
			}

			glm::vec3 fillColor;
			glm::vec3 borderColor;

			switch (type) {
				case CellType::Platform: // Высота 3: Светло-серый гранитный / каменный
					fillColor   = glm::vec3(0.70f, 0.72f, 0.76f);
					borderColor = glm::vec3(0.40f, 0.42f, 0.46f);
					break;
				case CellType::Ground: // Высота 2: Травянисто-зеленый
					fillColor   = glm::vec3(0.38f, 0.65f, 0.38f);
					borderColor = glm::vec3(0.20f, 0.42f, 0.20f);
					break;
				case CellType::Path: // Высота 1: Теплый песочно-глиняный
					fillColor   = glm::vec3(0.80f, 0.70f, 0.52f);
					borderColor = glm::vec3(0.52f, 0.42f, 0.28f);
					break;
				case CellType::Scenery: // Высота 0: Глубокий сланцевый / сине-серый
					fillColor   = glm::vec3(0.20f, 0.35f, 0.52f);
					borderColor = glm::vec3(0.11f, 0.20f, 0.32f);
					break;
				case CellType::Chasm: // Шурф / обрыв / провал: глубокая шахтная бездна
					fillColor   = glm::vec3(0.04f, 0.04f, 0.06f);
					borderColor = glm::vec3(0.02f, 0.02f, 0.03f);
					break;
				case CellType::Spawner: // Точка спавна врагов: Пурпурный / фиолетовый
					fillColor   = glm::vec3(0.65f, 0.35f, 0.75f);
					borderColor = glm::vec3(0.38f, 0.16f, 0.46f);
					break;
				case CellType::Base: // База игрока: Бирюзовый / циан
					fillColor   = glm::vec3(0.25f, 0.70f, 0.85f);
					borderColor = glm::vec3(0.12f, 0.42f, 0.55f);
					break;
				case CellType::Rail: // Рельсы / колія вагонетки: темный гравий/балласт со стальными рельсами
					fillColor   = glm::vec3(0.25f, 0.24f, 0.26f);
					borderColor = glm::vec3(0.14f, 0.13f, 0.16f);
					break;
				default:
					fillColor   = glm::vec3(0.38f, 0.65f, 0.38f);
					borderColor = glm::vec3(0.20f, 0.42f, 0.20f);
					break;
			}

			// 1. Внешний квадрат (окантовка темного оттенка того же цвета)
			renderer->drawSprite(whiteTexture, pixelPos, cellSizeVec, 0.0f, borderColor * color);

			// 2. Внутренний квадрат (основной цвет ячейки)
			renderer->drawSprite(whiteTexture, pixelPos + innerOffset, innerSize, 0.0f, fillColor * color);

			// 3. Для клетки обрыва (Chasm) рисуем глубокую сердцевину провала
			if (type == CellType::Chasm) {
				float depthPadding = std::max(2.0f, std::round(m_cellSize * 0.16f));
				glm::vec2 pitOffset(depthPadding, depthPadding);
				glm::vec2 pitSize(m_cellSize - 2.0f * depthPadding, m_cellSize - 2.0f * depthPadding);
				if (pitSize.x > 0.0f && pitSize.y > 0.0f) {
					renderer->drawSprite(whiteTexture, pixelPos + pitOffset, pitSize, 0.0f, glm::vec3(0.015f, 0.015f, 0.025f) * color);
				}
			}

			// 4. Для клетки рельсов (Rail) рисуем стилизованную колію с авто-тайлингом и бесшовными поворотами
			if (type == CellType::Rail) {
				bool up    = (y > 0 && getCellType(x, y - 1) == CellType::Rail);
				bool down  = (y < m_height - 1 && getCellType(x, y + 1) == CellType::Rail);
				bool left  = (x > 0 && getCellType(x - 1, y) == CellType::Rail);
				bool right = (x < m_width - 1 && getCellType(x + 1, y) == CellType::Rail);

				bool hasVert  = (up || down);
				bool hasHoriz = (left || right);

				float railOffset = std::max(3.0f, std::round(m_cellSize * 0.22f));
				float railThick  = std::max(2.0f, std::round(m_cellSize * 0.08f));
				float tiePadding = std::max(1.0f, std::round(m_cellSize * 0.08f));
				float tieThick   = std::max(2.0f, std::round(m_cellSize * 0.12f));

				glm::vec3 woodCol  = glm::vec3(0.42f, 0.28f, 0.18f) * color;
				glm::vec3 steelCol = glm::vec3(0.80f, 0.82f, 0.88f) * color;

				float r1 = railOffset;
				float r2 = m_cellSize - railOffset - railThick;

				if (!hasVert && !hasHoriz) {
					// Изолированная клетка: горизонтальные рельсы по умолчанию (вертикальные шпалы)
					for (int i = 0; i < 3; ++i) {
						float tieX = pixelPos.x + (m_cellSize * 0.25f) * (static_cast<float>(i) + 0.5f);
						renderer->drawSprite(whiteTexture, glm::vec2(tieX, pixelPos.y + tiePadding),
											 glm::vec2(tieThick, m_cellSize - 2.0f * tiePadding), 0.0f, woodCol);
					}
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r1), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
				} else if (hasVert && !hasHoriz) {
					// Чистая вертикаль
					for (int i = 0; i < 3; ++i) {
						float tieY = pixelPos.y + (m_cellSize * 0.25f) * (static_cast<float>(i) + 0.5f);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + tiePadding, tieY),
											 glm::vec2(m_cellSize - 2.0f * tiePadding, tieThick), 0.0f, woodCol);
					}
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y), glm::vec2(railThick, m_cellSize), 0.0f, steelCol);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y), glm::vec2(railThick, m_cellSize), 0.0f, steelCol);
				} else if (hasHoriz && !hasVert) {
					// Чистая горизонталь
					for (int i = 0; i < 3; ++i) {
						float tieX = pixelPos.x + (m_cellSize * 0.25f) * (static_cast<float>(i) + 0.5f);
						renderer->drawSprite(whiteTexture, glm::vec2(tieX, pixelPos.y + tiePadding),
											 glm::vec2(tieThick, m_cellSize - 2.0f * tiePadding), 0.0f, woodCol);
					}
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r1), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
				} else {
					// Повороты и перекрестки (есть и вертикаль, и горизонталь)
					if (up) {
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + tiePadding, pixelPos.y + m_cellSize * 0.15f),
											 glm::vec2(m_cellSize - 2.0f * tiePadding, tieThick), 0.0f, woodCol);
					}
					if (down) {
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + tiePadding, pixelPos.y + m_cellSize * 0.73f),
											 glm::vec2(m_cellSize - 2.0f * tiePadding, tieThick), 0.0f, woodCol);
					}
					if (left) {
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize * 0.15f, pixelPos.y + tiePadding),
											 glm::vec2(tieThick, m_cellSize - 2.0f * tiePadding), 0.0f, woodCol);
					}
					if (right) {
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize * 0.73f, pixelPos.y + tiePadding),
											 glm::vec2(tieThick, m_cellSize - 2.0f * tiePadding), 0.0f, woodCol);
					}
					float diagSz = std::max(4.0f, std::round(m_cellSize * 0.26f));
					renderer->drawSprite(whiteTexture, pixelPos + glm::vec2((m_cellSize - diagSz) * 0.5f),
										 glm::vec2(diagSz), 0.0f, woodCol);

					if (down && right && !up && !left) {
						// Поворот: Снизу направо
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r1), glm::vec2(railThick, m_cellSize - r1), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r1), glm::vec2(m_cellSize - r1, railThick), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y + r2), glm::vec2(railThick, m_cellSize - r2), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y + r2), glm::vec2(m_cellSize - r2, railThick), 0.0f, steelCol);
					} else if (down && left && !up && !right) {
						// Поворот: Снизу налево
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y + r1), glm::vec2(railThick, m_cellSize - r1), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r1), glm::vec2(r2 + railThick, railThick), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r2), glm::vec2(railThick, m_cellSize - r2), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(r1 + railThick, railThick), 0.0f, steelCol);
					} else if (up && right && !down && !left) {
						// Поворот: Сверху направо
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y), glm::vec2(railThick, r2 + railThick), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r2), glm::vec2(m_cellSize - r1, railThick), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y), glm::vec2(railThick, r1 + railThick), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y + r1), glm::vec2(m_cellSize - r2, railThick), 0.0f, steelCol);
					} else if (up && left && !down && !right) {
						// Поворот: Сверху налево
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y), glm::vec2(railThick, r2 + railThick), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(r2 + railThick, railThick), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y), glm::vec2(railThick, r1 + railThick), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r1), glm::vec2(r1 + railThick, railThick), 0.0f, steelCol);
					} else {
						// Т-образные соединения и перекрестки (3-4 соседа)
						if (up) {
							renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y), glm::vec2(railThick, m_cellSize * 0.5f), 0.0f, steelCol);
							renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y), glm::vec2(railThick, m_cellSize * 0.5f), 0.0f, steelCol);
						}
						if (down) {
							renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + m_cellSize * 0.5f), glm::vec2(railThick, m_cellSize * 0.5f), 0.0f, steelCol);
							renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y + m_cellSize * 0.5f), glm::vec2(railThick, m_cellSize * 0.5f), 0.0f, steelCol);
						}
						if (left) {
							renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r1), glm::vec2(m_cellSize * 0.5f, railThick), 0.0f, steelCol);
							renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(m_cellSize * 0.5f, railThick), 0.0f, steelCol);
						}
						if (right) {
							renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize * 0.5f, pixelPos.y + r1), glm::vec2(m_cellSize * 0.5f, railThick), 0.0f, steelCol);
							renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize * 0.5f, pixelPos.y + r2), glm::vec2(m_cellSize * 0.5f, railThick), 0.0f, steelCol);
						}
					}
				}
			}
		}
	}
}
// Динамический пересчет размера клеток при изменении размера окна с учетом нижней панели и верхнего отступа
void Grid::updateCellSize(int windowWidth, int windowHeight, float bottomMargin, float topMargin) {
	float usableHeight = std::max(1.0f, static_cast<float>(windowHeight) - bottomMargin - topMargin);

	// считаем размер клетки
	float sizeX = static_cast<float>(windowWidth) / static_cast<float>(m_width);
	float sizeY = usableHeight / static_cast<float>(m_height);

	// берем меньшее значение, чтобы клетки всегда оставались квадратными
	m_cellSize = std::min(sizeX, sizeY);

	// считаем фактический размер всей сетки в пикселях
	float actualGridWidth = m_cellSize * m_width;
	float actualGridHeight = m_cellSize * m_height;

	// высчитываем отступы: центрируем по горизонтали и в доступной области по вертикали с учетом верхнего бара
	m_offset.x = (static_cast<float>(windowWidth) - actualGridWidth) / 2.0f;
	m_offset.y = topMargin + (usableHeight - actualGridHeight) / 2.0f;
}


CellType Grid::getCellType(int gridX, int gridY) const {
	// 1. Сначала железная броня (чтобы не выйти за пределы 10 и 7)
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {

		// 2. ИМЕННО [gridX][gridY], а не наоборот!
		return m_grid[gridY][gridX];
	}

	// Если алгоритм спрашивает про клетку за экраном - говорим, что там стена
	return CellType::Scenery;
}