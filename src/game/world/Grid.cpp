#include "Grid.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include "resources/ResourceManager.h"
#include <algorithm>
#include <cmath>

// конструктор создает пустую сетку заданого размера
Grid::Grid(int width, int height, float cellSize, glm::vec2 offset) {
	m_width = width;
	m_height = height;
	m_cellSize = cellSize;
	m_offset = offset;
	m_grid.resize(m_height, std::vector<CellType>(m_width, CellType::Ground));
	m_originalGrid.resize(m_height, std::vector<CellType>(m_width, CellType::Ground));
	m_noBuildMask.assign(m_height, std::vector<bool>(m_width, false));
}

void Grid::saveOriginalGrid() {
	m_originalGrid = m_grid;
}

CellType Grid::getOriginalCellType(int gridX, int gridY) const {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		return m_originalGrid[gridY][gridX];
	}
	if (m_hasSidePockets) {
		auto it = m_outerCells.find({gridX, gridY});
		if (it != m_outerCells.end()) return it->second;
		return CellType::Empty;
	}
	return CellType::Ground;
}

// Перевод из индексов клетки в пиксели экрана
glm::vec2 Grid::gridToPixel(int gridX, int gridY) const {
	return glm::vec2(gridX * m_cellSize, gridY * m_cellSize) + m_offset;
}

// Перевод из пикселей экрана в индексы клетки сетки (х,у)
glm::ivec2 Grid::pixelToGrid(glm::vec2 pixelPos) const {
	float localX = pixelPos.x - m_offset.x;
	float localY = pixelPos.y - m_offset.y;

	int gridX = static_cast<int>(std::floor(localX / m_cellSize));
	int gridY = static_cast<int>(std::floor(localY / m_cellSize));

	return glm::ivec2(gridX, gridY);
}

// Проверяем можно ли построить башню на клетке с этими индексами
bool Grid::canBuildAt(int gridX, int gridY) const {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		CellType type = getCellType(gridX, gridY);
		if (type == CellType::Chasm || type == CellType::Rail) {
			return false;
		}
		if (isNoBuild(gridX, gridY)) return false;
		return (type == CellType::Ground || type == CellType::Platform);
	}

	if (m_hasSidePockets) {
		auto it = m_outerCells.find({gridX, gridY});
		if (it == m_outerCells.end()) return false;
		CellType type = it->second;
		if (type == CellType::Chasm || type == CellType::Rail || type == CellType::Empty || type == CellType::Scenery) {
			return false;
		}
		if (isNoBuild(gridX, gridY)) return false;
		return (type == CellType::Ground || type == CellType::Platform);
	}

	return false;
}

// Принудительно меняем тип конкретной ячейки
void Grid::setCellType(int gridX, int gridY, CellType type, bool bumpVersion) {
	bool changed = false;
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		if (m_grid[gridY][gridX] != type) {
			m_grid[gridY][gridX] = type;
			changed = true;
		}
	} else if (m_hasSidePockets) {
		if (type == CellType::Empty) {
			if (m_outerCells.erase({gridX, gridY}) > 0) {
				changed = true;
			}
		} else {
			auto it = m_outerCells.find({gridX, gridY});
			if (it == m_outerCells.end() || it->second != type) {
				m_outerCells[{gridX, gridY}] = type;
				changed = true;
			}
		}
	}
	if (changed && bumpVersion) {
		m_version++;
	}
}

// Отрисовка всей карты ячейка за ячейкой
void Grid::renderCellVisual(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	int x, int y, CellType type,
	glm::vec2 pixelPos, glm::vec3 color,
	bool isMineTheme, bool isCanalTheme, bool isCinnabarTheme) const
{
	glm::vec2 cellSizeVec(m_cellSize, m_cellSize);
	float borderWidth = std::max(1.0f, std::round(m_cellSize * 0.04f));
	glm::vec2 innerOffset(borderWidth, borderWidth);
	glm::vec2 innerSize(m_cellSize - 2.0f * borderWidth, m_cellSize - 2.0f * borderWidth);

	glm::vec3 fillColor;
	glm::vec3 borderColor;

		switch (type) {
			case CellType::Platform:
				if (isCanalTheme) {
					fillColor   = glm::vec3(0.32f, 0.58f, 0.30f);
					borderColor = glm::vec3(0.20f, 0.40f, 0.18f);
				} else if (isMineTheme) {
					fillColor   = glm::vec3(0.52f, 0.46f, 0.40f);
					borderColor = glm::vec3(0.34f, 0.30f, 0.26f);
				} else if (isCinnabarTheme) {
					fillColor   = glm::vec3(0.28f, 0.24f, 0.24f);
					borderColor = glm::vec3(0.18f, 0.15f, 0.16f);
				} else {
					fillColor   = glm::vec3(0.70f, 0.72f, 0.76f);
					borderColor = glm::vec3(0.40f, 0.42f, 0.46f);
				}
				break;
			case CellType::Ground:
				if (isCanalTheme) {
					fillColor   = glm::vec3(0.22f, 0.23f, 0.25f);
					borderColor = glm::vec3(0.14f, 0.15f, 0.17f);
				} else if (isMineTheme) {
					fillColor   = glm::vec3(0.46f, 0.29f, 0.17f);
					borderColor = glm::vec3(0.36f, 0.22f, 0.12f);
				} else if (isCinnabarTheme) {
					fillColor   = glm::vec3(0.165f, 0.176f, 0.212f);
					borderColor = glm::vec3(0.110f, 0.118f, 0.145f);
				} else {
					fillColor   = glm::vec3(0.38f, 0.65f, 0.38f);
					borderColor = glm::vec3(0.20f, 0.42f, 0.20f);
				}
				break;
			case CellType::Path:
				if (isMineTheme) {
					fillColor   = glm::vec3(0.48f, 0.31f, 0.18f);
					borderColor = glm::vec3(0.36f, 0.22f, 0.12f);
				} else if (isCinnabarTheme) {
					fillColor   = glm::vec3(0.235f, 0.208f, 0.218f);
					borderColor = glm::vec3(0.150f, 0.132f, 0.142f);
				} else {
					fillColor   = glm::vec3(0.80f, 0.70f, 0.52f);
					borderColor = glm::vec3(0.52f, 0.42f, 0.28f);
				}
				break;
			case CellType::Scenery:
				if (isMineTheme) {
					fillColor   = glm::vec3(0.52f, 0.34f, 0.19f); // Деревянный ящик в теме шахты
					borderColor = glm::vec3(0.28f, 0.16f, 0.09f);
				} else if (isCinnabarTheme) {
					fillColor   = glm::vec3(0.195f, 0.205f, 0.245f); // Целик сланца с жилой киновари
					borderColor = glm::vec3(0.085f, 0.090f, 0.112f);
				} else {
					fillColor   = glm::vec3(0.20f, 0.35f, 0.52f); // Классическая стена (Канал, стандарт, дефолт)
					borderColor = glm::vec3(0.11f, 0.20f, 0.32f);
				}
				break;
			case CellType::Chasm:
				fillColor   = glm::vec3(0.04f, 0.04f, 0.06f);
				borderColor = glm::vec3(0.02f, 0.02f, 0.03f);
				break;
			case CellType::Spawner:
				fillColor   = glm::vec3(0.65f, 0.35f, 0.75f);
				borderColor = glm::vec3(0.38f, 0.16f, 0.46f);
				break;
			case CellType::Base:
				fillColor   = glm::vec3(0.25f, 0.70f, 0.85f);
				borderColor = glm::vec3(0.12f, 0.42f, 0.55f);
				break;
			case CellType::Rail:
				fillColor   = glm::vec3(0.25f, 0.24f, 0.26f);
				borderColor = glm::vec3(0.14f, 0.13f, 0.16f);
				break;
			default:
				return;
		}

		// 1. Внешний квадрат (окантовка темного оттенка того же цвета)
		renderer->drawSprite(whiteTexture, pixelPos, cellSizeVec, 0.0f, borderColor * color);

		// 2. Внутренний квадрат (основной цвет ячейки)
		renderer->drawSprite(whiteTexture, pixelPos + innerOffset, innerSize, 0.0f, fillColor * color);

		// 2.1. Для темы "Вход в шахту": текстура пыли и камешков на грунте (Ground и Path)
		if (isMineTheme && (type == CellType::Ground || type == CellType::Path)) {
			uint32_t seed = static_cast<uint32_t>(std::abs(x * 7919 + y * 65537));
			float p1x = ((seed % 50) + 10) * 0.01f;
			float p1y = (((seed / 50) % 50) + 10) * 0.01f;
			float p2x = (((seed / 2500) % 50) + 20) * 0.01f;
			float p2y = (((seed / 125000) % 50) + 20) * 0.01f;
			float pSz = std::max(2.0f, std::round(m_cellSize * 0.06f));
			renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(p1x * m_cellSize, p1y * m_cellSize), glm::vec2(pSz, pSz), 0.0f, glm::vec3(0.35f, 0.20f, 0.11f) * color);
			renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(p2x * m_cellSize, p2y * m_cellSize), glm::vec2(pSz * 0.8f, pSz * 0.8f), 0.0f, glm::vec3(0.56f, 0.36f, 0.22f) * color);
		}

		// 2.2. Клетка-препятствие "Ящики" (Scenery в теме "Вход в шахту"):
		// Вид строго сверху: окантовка по периметру, продольные доски и диагональная планка
		if (type == CellType::Scenery && isMineTheme) {
			float frameThick = std::max(3.0f, std::round(m_cellSize * 0.12f));
			glm::vec3 darkWood = glm::vec3(0.26f, 0.15f, 0.08f) * color;
			glm::vec3 plankCol1 = glm::vec3(0.55f, 0.35f, 0.20f) * color;
			glm::vec3 plankCol2 = glm::vec3(0.48f, 0.30f, 0.17f) * color;
			glm::vec3 braceCol = glm::vec3(0.44f, 0.27f, 0.15f) * color;
			glm::vec3 metalCol = glm::vec3(0.38f, 0.40f, 0.44f) * color;
			glm::vec3 rivetCol = glm::vec3(0.85f, 0.86f, 0.90f) * color;

			glm::vec2 boxInnerPos = pixelPos + glm::vec2(frameThick);
			glm::vec2 boxInnerSize(m_cellSize - 2.0f * frameThick, m_cellSize - 2.0f * frameThick);

			float plankH = boxInnerSize.y / 3.0f;
			for (int p = 0; p < 3; ++p) {
				glm::vec3 pCol = (p == 1) ? plankCol1 : plankCol2;
				renderer->drawSprite(whiteTexture, glm::vec2(boxInnerPos.x, boxInnerPos.y + static_cast<float>(p) * plankH), glm::vec2(boxInnerSize.x, plankH), 0.0f, pCol);
				if (p > 0) {
					renderer->drawSprite(whiteTexture, glm::vec2(boxInnerPos.x, boxInnerPos.y + static_cast<float>(p) * plankH - 1.0f), glm::vec2(boxInnerSize.x, 2.0f), 0.0f, darkWood);
				}
			}

			float diagLen = m_cellSize * 1.15f;
			float diagThick = std::max(4.0f, std::round(m_cellSize * 0.13f));
			glm::vec2 centerPos = pixelPos + glm::vec2(m_cellSize * 0.5f);
			renderer->drawSprite(whiteTexture, centerPos - glm::vec2(diagLen * 0.5f, diagThick * 0.5f) + glm::vec2(1.5f, 1.5f), glm::vec2(diagLen, diagThick), 45.0f, darkWood);
			renderer->drawSprite(whiteTexture, centerPos - glm::vec2(diagLen * 0.5f, diagThick * 0.5f), glm::vec2(diagLen, diagThick), 45.0f, braceCol);

			renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(m_cellSize, frameThick), 0.0f, darkWood);
			renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + m_cellSize - frameThick), glm::vec2(m_cellSize, frameThick), 0.0f, darkWood);
			renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(frameThick, m_cellSize), 0.0f, darkWood);
			renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize - frameThick, pixelPos.y), glm::vec2(frameThick, m_cellSize), 0.0f, darkWood);

			float bracketSz = std::max(5.0f, std::round(m_cellSize * 0.16f));
			float rivetSz = std::max(2.0f, std::round(m_cellSize * 0.05f));
		auto drawCornerBracket = [&](glm::vec2 bPos, glm::vec2 rPos) {
				renderer->drawSprite(whiteTexture, bPos, glm::vec2(bracketSz), 0.0f, metalCol);
				renderer->drawSprite(whiteTexture, rPos, glm::vec2(rivetSz), 0.0f, rivetCol);
			};
			drawCornerBracket(pixelPos, pixelPos + glm::vec2(bracketSz * 0.25f));
			drawCornerBracket(glm::vec2(pixelPos.x + m_cellSize - bracketSz, pixelPos.y), glm::vec2(pixelPos.x + m_cellSize - bracketSz * 0.75f, pixelPos.y + bracketSz * 0.25f));
			drawCornerBracket(glm::vec2(pixelPos.x, pixelPos.y + m_cellSize - bracketSz), glm::vec2(pixelPos.x + bracketSz * 0.25f, pixelPos.y + m_cellSize - bracketSz * 0.75f));
			drawCornerBracket(glm::vec2(pixelPos.x + m_cellSize - bracketSz, pixelPos.y + m_cellSize - bracketSz), glm::vec2(pixelPos.x + m_cellSize - bracketSz * 0.75f, pixelPos.y + m_cellSize - bracketSz * 0.75f));
		}

		// 2.3. Для темы "Киноварный штрек" (cinnabar_mine): каменная крошка на полу и целики породы с киноварью
		if (isCinnabarTheme) {
			uint32_t seed = static_cast<uint32_t>(std::abs(x * 7919 + y * 65537));
			if (type == CellType::Ground || type == CellType::Path) {
				float p1x = static_cast<float>((seed % 52) + 12) * 0.01f;
				float p1y = static_cast<float>(((seed / 53) % 52) + 12) * 0.01f;
				float p2x = static_cast<float>(((seed / 2809) % 48) + 22) * 0.01f;
				float p2y = static_cast<float>(((seed / 134832) % 48) + 22) * 0.01f;
				float pSz = std::max(2.0f, std::round(m_cellSize * 0.055f));
				renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(p1x * m_cellSize, p1y * m_cellSize), glm::vec2(pSz, pSz), 0.0f, glm::vec3(0.11f, 0.12f, 0.15f) * color);
				if ((seed % 5) == 0) {
					// Редкая искорка рудной крошки киновари на полу штрека
					renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(p2x * m_cellSize, p2y * m_cellSize), glm::vec2(pSz * 0.85f, pSz * 0.85f), 0.0f, glm::vec3(0.68f, 0.16f, 0.18f) * color);
				} else {
					renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(p2x * m_cellSize, p2y * m_cellSize), glm::vec2(pSz * 0.8f, pSz * 0.8f), 0.0f, glm::vec3(0.24f, 0.25f, 0.30f) * color);
				}
			} else if (type == CellType::Platform) {
				// Укрепленный строительный уступ с деревянной окантовкой по углам
				float studSz = std::max(3.0f, std::round(m_cellSize * 0.09f));
				float pad = borderWidth + 2.0f;
				glm::vec3 timberCol = glm::vec3(0.42f, 0.28f, 0.17f) * color;
				renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(pad, pad), glm::vec2(studSz), 0.0f, timberCol);
				renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(m_cellSize - pad - studSz, pad), glm::vec2(studSz), 0.0f, timberCol);
				renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(pad, m_cellSize - pad - studSz), glm::vec2(studSz), 0.0f, timberCol);
				renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(m_cellSize - pad - studSz, m_cellSize - pad - studSz), glm::vec2(studSz), 0.0f, timberCol);
			} else if (type == CellType::Scenery) {
				// Каменный целик сланца и кварцита, прорезанный жилой алой киновари и каплями ртути
				float bevel = std::max(3.0f, std::round(m_cellSize * 0.10f));
				glm::vec3 topBevel = glm::vec3(0.28f, 0.29f, 0.35f) * color;
				glm::vec3 botBevel = glm::vec3(0.11f, 0.12f, 0.15f) * color;
				glm::vec3 coreRock = glm::vec3(0.165f, 0.175f, 0.215f) * color;

				renderer->drawSprite(whiteTexture, pixelPos + innerOffset, glm::vec2(innerSize.x, bevel), 0.0f, topBevel);
				renderer->drawSprite(whiteTexture, pixelPos + innerOffset, glm::vec2(bevel, innerSize.y), 0.0f, topBevel);
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + innerOffset.x, pixelPos.y + m_cellSize - innerOffset.y - bevel), glm::vec2(innerSize.x, bevel), 0.0f, botBevel);
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize - innerOffset.x - bevel, pixelPos.y + innerOffset.y), glm::vec2(bevel, innerSize.y), 0.0f, botBevel);

				glm::vec2 corePos = pixelPos + innerOffset + glm::vec2(bevel);
				glm::vec2 coreSz = innerSize - glm::vec2(bevel * 2.0f);
				if (coreSz.x > 2.0f && coreSz.y > 2.0f) {
					renderer->drawSprite(whiteTexture, corePos, coreSz, 0.0f, coreRock);
				}

				float angles[4] = { 28.0f, -34.0f, 52.0f, -48.0f };
				float veinAngle = angles[seed % 4];
				float veinLen = m_cellSize * 0.68f;
				float veinThick = std::max(3.0f, std::round(m_cellSize * 0.11f));
				glm::vec2 cellCenter = pixelPos + glm::vec2(m_cellSize * 0.5f);

				renderer->drawSprite(whiteTexture, cellCenter - glm::vec2(veinLen * 0.5f, (veinThick + 3.0f) * 0.5f), glm::vec2(veinLen, veinThick + 3.0f), veinAngle, glm::vec3(0.06f, 0.04f, 0.06f) * color);
				renderer->drawSprite(whiteTexture, cellCenter - glm::vec2(veinLen * 0.5f, veinThick * 0.5f), glm::vec2(veinLen, veinThick), veinAngle, glm::vec3(0.72f, 0.13f, 0.16f) * color);
				renderer->drawSprite(whiteTexture, cellCenter - glm::vec2(veinLen * 0.26f, veinThick * 0.28f), glm::vec2(veinLen * 0.52f, std::max(2.0f, veinThick * 0.55f)), veinAngle, glm::vec3(0.95f, 0.24f, 0.26f) * color);

				if ((seed % 3) != 0) {
					float hgSz = std::max(2.0f, std::round(m_cellSize * 0.065f));
					float offX = ((seed % 2) == 0) ? (m_cellSize * 0.16f) : (-m_cellSize * 0.18f);
					renderer->drawSprite(whiteTexture, cellCenter + glm::vec2(offX, -m_cellSize * 0.14f), glm::vec2(hgSz, hgSz), 0.0f, glm::vec3(0.85f, 0.90f, 0.96f) * color);
				}
			}
		}

		// 3. Для клетки обрыва (Chasm) рисуем глубокую сердцевину провала
		if (type == CellType::Chasm) {
			float depthPadding = std::max(2.0f, std::round(m_cellSize * 0.16f));
			glm::vec2 pitOffset(depthPadding, depthPadding);
			glm::vec2 pitSize(m_cellSize - 2.0f * depthPadding, m_cellSize - 2.0f * depthPadding);
			if (pitSize.x > 0.0f && pitSize.y > 0.0f) {
				renderer->drawSprite(whiteTexture, pixelPos + pitOffset, pitSize, 0.0f, glm::vec3(0.015f, 0.015f, 0.025f) * color);
			}
		}

		// 4. Для клетки рельсов (Rail) рисуем стилизованную колію с авто-тайлингом
		if (type == CellType::Rail) {
			bool up    = (getCellType(x, y - 1) == CellType::Rail);
			bool down  = (getCellType(x, y + 1) == CellType::Rail);
			bool left  = (getCellType(x - 1, y) == CellType::Rail);
			bool right = (getCellType(x + 1, y) == CellType::Rail);

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
				for (int i = 0; i < 3; ++i) {
					float tieX = pixelPos.x + (m_cellSize * 0.25f) * (static_cast<float>(i) + 0.5f);
					renderer->drawSprite(whiteTexture, glm::vec2(tieX, pixelPos.y + tiePadding),
										 glm::vec2(tieThick, m_cellSize - 2.0f * tiePadding), 0.0f, woodCol);
				}
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r1), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
			} else if (hasVert && !hasHoriz) {
				for (int i = 0; i < 3; ++i) {
					float tieY = pixelPos.y + (m_cellSize * 0.25f) * (static_cast<float>(i) + 0.5f);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + tiePadding, tieY),
										 glm::vec2(m_cellSize - 2.0f * tiePadding, tieThick), 0.0f, woodCol);
				}
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y), glm::vec2(railThick, m_cellSize), 0.0f, steelCol);
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y), glm::vec2(railThick, m_cellSize), 0.0f, steelCol);
			} else if (hasHoriz && !hasVert) {
				for (int i = 0; i < 3; ++i) {
					float tieX = pixelPos.x + (m_cellSize * 0.25f) * (static_cast<float>(i) + 0.5f);
					renderer->drawSprite(whiteTexture, glm::vec2(tieX, pixelPos.y + tiePadding),
										 glm::vec2(tieThick, m_cellSize - 2.0f * tiePadding), 0.0f, woodCol);
				}
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r1), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
			} else {
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
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r1), glm::vec2(railThick, m_cellSize - r1), 0.0f, steelCol);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r1), glm::vec2(m_cellSize - r1, railThick), 0.0f, steelCol);
				} else if (down && left && !up && !right) {
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y + r1), glm::vec2(railThick, m_cellSize - r1), 0.0f, steelCol);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r1), glm::vec2(r2 + railThick, railThick), 0.0f, steelCol);
				} else if (up && right && !down && !left) {
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y), glm::vec2(railThick, r1 + railThick), 0.0f, steelCol);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r2), glm::vec2(m_cellSize - r1, railThick), 0.0f, steelCol);
				} else if (up && left && !down && !right) {
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y), glm::vec2(railThick, r1 + railThick), 0.0f, steelCol);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(r2 + railThick, railThick), 0.0f, steelCol);
				} else {
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

void Grid::drawBaseCells(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	glm::vec3 color,
	const std::string& backgroundPreset) const
{
	if (!renderer || !whiteTexture) return;

	const bool isMineTheme = (backgroundPreset == "mine_entrance" || backgroundPreset == "mine");
	const bool isCanalTheme = (backgroundPreset == "pipes_canal" || backgroundPreset == "channel");
	const bool isCinnabarTheme = (backgroundPreset == "cinnabar_mine" || backgroundPreset == "cinnabar");

	// 1. Отрисовка базовых клеток основной матрицы (без Scenery)
	for (int y = 0; y < m_height; ++y) {
		for (int x = 0; x < m_width; ++x) {
			CellType type = m_grid[y][x];
			if (type == CellType::Tower) {
				type = m_originalGrid[y][x];
			}
			if (type == CellType::Spawner || type == CellType::Base) {
				type = CellType::Ground;
			}
			if (type == CellType::Scenery || type == CellType::Empty) continue;
			renderCellVisual(renderer, whiteTexture, x, y, type, gridToPixel(x, y), color, isMineTheme, isCanalTheme, isCinnabarTheme);
		}
	}

	// 2. Отрисовка внешних базовых клеток (Грунт, Дорога, Платформа, Шурф, Рельсы) под фоновым окружением
	for (const auto& [pos, type] : m_outerCells) {
		CellType ct = type;
		if (ct == CellType::Spawner || ct == CellType::Base) {
			ct = CellType::Ground;
		}
		if (ct == CellType::Scenery || ct == CellType::Empty) continue;
		renderCellVisual(renderer, whiteTexture, pos.first, pos.second, ct, gridToPixel(pos.first, pos.second), color, isMineTheme, isCanalTheme, isCinnabarTheme);
	}
}

void Grid::drawEditorGridLines(SpriteRenderer* renderer,
	const Texture2D* whiteTexture) const
{
	if (!renderer || !whiteTexture) return;

	// При включенных свободных границах рисуем мягкую полупрозрачную сетку вокруг матрицы поверх фона
	if (m_hasSidePockets) {
		float gBorder = std::max(1.0f, std::round(m_cellSize * 0.02f));
		glm::vec4 gBorderCol(1.0f, 1.0f, 1.0f, 0.22f);
		int padX = 14;
		int padY = 8;
		for (int y = -padY; y < m_height + padY; ++y) {
			for (int x = -padX; x < m_width + padX; ++x) {
				if (x >= 0 && x < m_width && y >= 0 && y < m_height) continue;
				glm::vec2 pixelPos = gridToPixel(x, y);
				renderer->drawSpriteRGBA(whiteTexture, pixelPos, glm::vec2(m_cellSize, gBorder), 0.0f, gBorderCol);
				renderer->drawSpriteRGBA(whiteTexture, pixelPos + glm::vec2(0.0f, m_cellSize - gBorder), glm::vec2(m_cellSize, gBorder), 0.0f, gBorderCol);
				renderer->drawSpriteRGBA(whiteTexture, pixelPos, glm::vec2(gBorder, m_cellSize), 0.0f, gBorderCol);
				renderer->drawSpriteRGBA(whiteTexture, pixelPos + glm::vec2(m_cellSize - gBorder, 0.0f), glm::vec2(gBorder, m_cellSize), 0.0f, gBorderCol);
			}
		}
	}
}

void Grid::drawObstacleCells(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	glm::vec3 color,
	const std::string& backgroundPreset) const
{
	if (!renderer || !whiteTexture) return;

	const bool isMineTheme = (backgroundPreset == "mine_entrance" || backgroundPreset == "mine");
	const bool isCanalTheme = (backgroundPreset == "pipes_canal" || backgroundPreset == "channel");
	const bool isCinnabarTheme = (backgroundPreset == "cinnabar_mine" || backgroundPreset == "cinnabar");

	// 1. Отрисовка препятствий Scenery ("Ящики" / "Стены" / "Киноварь") в матрице
	for (int y = 0; y < m_height; ++y) {
		for (int x = 0; x < m_width; ++x) {
			CellType type = m_grid[y][x];
			if (type == CellType::Tower) {
				type = m_originalGrid[y][x];
			}
			if (type == CellType::Scenery) {
				renderCellVisual(renderer, whiteTexture, x, y, type, gridToPixel(x, y), color, isMineTheme, isCanalTheme, isCinnabarTheme);
			}
		}
	}

	// 2. Отрисовка препятствий Scenery во внешних клетках
	for (const auto& [pos, type] : m_outerCells) {
		if (type == CellType::Scenery) {
			renderCellVisual(renderer, whiteTexture, pos.first, pos.second, type, gridToPixel(pos.first, pos.second), color, isMineTheme, isCanalTheme, isCinnabarTheme);
		}
	}
}

void Grid::drawOuterNoBuildOverlay(SpriteRenderer* renderer,
	const Texture2D* whiteTexture) const
{
	if (!renderer || !whiteTexture) return;

	for (const auto& pos : m_outerNoBuild) {
		glm::vec2 pPos = gridToPixel(pos.first, pos.second);
		float borderW = std::max(2.0f, std::round(m_cellSize * 0.04f));
		glm::vec4 bgCol(1.0f, 0.20f, 0.20f, 0.18f);
		glm::vec4 borderCol(0.95f, 0.30f, 0.30f, 0.75f);
		glm::vec4 crossCol(0.95f, 0.25f, 0.25f, 0.85f);
		float barLen = m_cellSize * 0.70f;
		float barThick = std::max(2.0f, std::round(m_cellSize * 0.05f));

		renderer->drawSpriteRGBA(whiteTexture, pPos + glm::vec2(1.0f), glm::vec2(m_cellSize - 2.0f), 0.0f, bgCol);
		renderer->drawSpriteRGBA(whiteTexture, pPos, glm::vec2(m_cellSize, borderW), 0.0f, borderCol);
		renderer->drawSpriteRGBA(whiteTexture, pPos + glm::vec2(0.0f, m_cellSize - borderW), glm::vec2(m_cellSize, borderW), 0.0f, borderCol);
		renderer->drawSpriteRGBA(whiteTexture, pPos, glm::vec2(borderW, m_cellSize), 0.0f, borderCol);
		renderer->drawSpriteRGBA(whiteTexture, pPos + glm::vec2(m_cellSize - borderW, 0.0f), glm::vec2(borderW, m_cellSize), 0.0f, borderCol);

		glm::vec2 center = pPos + glm::vec2(m_cellSize * 0.5f);
		renderer->drawSpriteRGBA(whiteTexture, center - glm::vec2(barLen * 0.5f, barThick * 0.5f), glm::vec2(barLen, barThick), 45.0f, crossCol);
		renderer->drawSpriteRGBA(whiteTexture, center - glm::vec2(barLen * 0.5f, barThick * 0.5f), glm::vec2(barLen, barThick), -45.0f, crossCol);
	}
}

void Grid::draw(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	glm::vec3 color,
	const std::string& backgroundPreset)
{
	drawBaseCells(renderer, whiteTexture, color, backgroundPreset);
	drawObstacleCells(renderer, whiteTexture, color, backgroundPreset);
	drawEditorGridLines(renderer, whiteTexture);
	drawOuterNoBuildOverlay(renderer, whiteTexture);
}

// Динамический пересчет размера клеток при изменении размера окна с учетом нижней панели и верхнего отступа
void Grid::updateCellSize(int windowWidth, int windowHeight, float bottomMargin, float topMargin) {
	float usableHeight = std::max(1.0f, static_cast<float>(windowHeight) - bottomMargin - topMargin);
	float sizeX = static_cast<float>(windowWidth) / static_cast<float>(m_width);
	float sizeY = usableHeight / static_cast<float>(m_height);
	m_cellSize = std::min(sizeX, sizeY);
	float actualGridWidth = m_cellSize * m_width;
	float actualGridHeight = m_cellSize * m_height;
	m_offset.x = (static_cast<float>(windowWidth) - actualGridWidth) / 2.0f;
	m_offset.y = topMargin + (usableHeight - actualGridHeight) / 2.0f;
}

CellType Grid::getCellType(int gridX, int gridY) const {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		return m_grid[gridY][gridX];
	}

	if (m_hasSidePockets) {
		auto it = m_outerCells.find({gridX, gridY});
		if (it != m_outerCells.end()) {
			return it->second;
		}
		return CellType::Empty;
	}

	return CellType::Scenery;
}

void Grid::setNoBuildMask(const std::vector<std::vector<bool>>& mask) {
	m_noBuildMask.assign(m_height, std::vector<bool>(m_width, false));
	for (int y = 0; y < (int)mask.size() && y < m_height; ++y) {
		for (int x = 0; x < (int)mask[y].size() && x < m_width; ++x) {
			m_noBuildMask[y][x] = mask[y][x];
		}
	}
}

bool Grid::isNoBuild(int gridX, int gridY) const {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		return m_noBuildMask[gridY][gridX];
	}
	if (m_hasSidePockets) {
		return m_outerNoBuild.find({gridX, gridY}) != m_outerNoBuild.end();
	}
	return false;
}

void Grid::setNoBuildAt(int gridX, int gridY, bool value) {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		m_noBuildMask[gridY][gridX] = value;
	} else if (m_hasSidePockets) {
		if (value) {
			m_outerNoBuild.insert({gridX, gridY});
		} else {
			m_outerNoBuild.erase({gridX, gridY});
		}
	}
}

void Grid::removeNoBuildAt(int gridX, int gridY) {
	setNoBuildAt(gridX, gridY, false);
}
