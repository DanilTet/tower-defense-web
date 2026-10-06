#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <memory>
#include "../shaders/ShaderProgram.h"

//Структура где хранится информация о символе
struct Character {
	unsigned int TextureID; // Айди текстурі глифа в OpenGL
	glm::ivec2 Size; // размер глифа (ширина, вісота)
	glm::ivec2 Bearing; // смещение от линии шрифта до левого верхнего угла глифа
	unsigned int Advance; // смещение до следующей буквы
};

class TextRenderer {
public:
	// хранилище всех загруженых букв
	std::map<char32_t, Character> Characters;
	std::shared_ptr<ShaderProgram> TextShader;

	// конструктор
	TextRenderer(std::shared_ptr<ShaderProgram> shader, int windowWidth, int windowHeight);
	// декструктор
	~TextRenderer();

	// метод загрузки шрифта
	bool Load(const std::string& fontPath, unsigned int fontSize);

	// метод отрисовки текста
	void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color);

	// Метод для обновления матрицы при ресайзе окна
	void updateProjection(const glm::mat4& projection);

	// функция для вычесления ширины текста
	float CalculateTextWidth(const std::string& text, float scale);
private:
	unsigned int VAO, VBO;
};