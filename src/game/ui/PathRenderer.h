#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class SpriteRenderer;
class Texture2D;
class Grid;

class PathVisualizer {
public:
    void renderPathArrows(
        SpriteRenderer* renderer,// для рендера
        std::shared_ptr<Texture2D> arrowTex, // текстура стрелки
        const std::vector<glm::ivec2>& levelPath, // путь
        const Grid& gameGrid, // сетка
        glm::vec3 arrowColor = glm::vec3(1.0f, 1.0f, 1.0f)
    );
	void update(float dt, float cellSize);
private:
	float m_pathAnimationTimer = 0.0f; // таймер анимации стрелочек
};