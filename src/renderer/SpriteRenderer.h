#pragma once

#include <glm/glm.hpp>
#include <memory>

class ShaderProgram;
class Texture2D;

// четенькая структура которая описывает кусок текстуры в атласе 
struct SpriteUV {
	glm::vec2 uvMin = glm::vec2(0.0f, 0.0f); // левый верхний угол
	glm::vec2 uvMax = glm::vec2(1.0f, 1.0f); // правый нижний угол

	// метод который сам считает дроби из пикселей
	static SpriteUV fromPixels(int x, int y, int width, int height, int atlasWidth, int atlasHeight) {
		SpriteUV uv;
		uv.uvMin.x = static_cast<float>(x) / atlasWidth;
		uv.uvMin.y = static_cast<float>(y) / atlasHeight;
		uv.uvMax.x = static_cast<float>(x + width) / atlasWidth;
		uv.uvMax.y = static_cast<float>(y + height) / atlasHeight;
		return uv;
	}
};

// структура вершины
struct SpriteVertex {
	glm::vec2 position; // позиция 
	glm::vec2 texCoords; // текстура
	glm::vec4 color; // цвет
};



class SpriteRenderer
{
public:
	explicit SpriteRenderer(std::shared_ptr<ShaderProgram> shader);
	~SpriteRenderer();

	SpriteRenderer(const SpriteRenderer&) = delete;
	SpriteRenderer& operator=(const SpriteRenderer&) = delete;

	SpriteRenderer(SpriteRenderer&& other) noexcept;
	SpriteRenderer& operator=(SpriteRenderer&& other) noexcept;


	void setProjection(const glm::mat4& projection);

	// новые четенькие методы управления пакетом
	void beginBatch();
	void endBatch();
	void flush();

	void drawSprite(const std::shared_ptr<Texture2D>& texture,
		glm::vec2 position,
		glm::vec2 size = glm::vec2(10.0f, 10.0f),
		float rotation = 0.0f,
		glm::vec3 color = glm::vec3(1.0f),
		SpriteUV uv = SpriteUV());

	void drawSpriteRGBA(const std::shared_ptr<Texture2D>& texture,
		glm::vec2 position,
		glm::vec2 size = glm::vec2(10.0f, 10.0f),
		float rotation = 0.0f,
		glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
		SpriteUV uv = SpriteUV());

private:
	std::shared_ptr<ShaderProgram> m_shader;

	unsigned int m_quadVAO = 0;
	unsigned int m_vbo = 0;
	unsigned int m_ebo = 0;

	//  данные для батчинга
	static const int MAX_SPRITES = 10000; // сколько справтов можем нарисовать за 1 вызов
	static const int MAX_VERTICES = MAX_SPRITES * 4;
	static const int MAX_INDICES = MAX_SPRITES * 6;

	std::vector<SpriteVertex> m_vertices;
	std::shared_ptr<Texture2D> m_currentTexture = nullptr;
	int m_spriteCount = 0;

	void initRenderData();
};