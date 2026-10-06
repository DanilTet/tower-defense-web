#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

class SpriteRenderer;
class Texture2D;
class Grid;

// конфиг для создания єфекта
struct ParticleEmitterProps {
	glm::vec2 position; // позиция
	glm::vec2 velocityDir; // направление кстати если бахнуть (0,0) то во все стороны разлетаются
	float velocityVariation; // сила разброса
	glm::vec3 startColor; // цвет при рождении
	glm::vec3 endColor; // цвет при смерти
	float startSize; // начальній размер
	float endSize; // конченый размер
	float lifeTime; // время жизни в секундах
	int spawnCount = 1; // сколько частиц спавним
};

struct Particle {
	glm::vec2 position;
	glm::vec2 velocity;
	glm::vec3 startColor, endColor;
	float startSize, endSize;
	float lifeTime;
	float lifeRemaining;
	bool active = false; // флаг для пула обьектов
};


class ParticleSystem {
public:
	// в пуле будет 2000 частиц
	ParticleSystem(size_t maxParticles = 2000);

	void update(float dt);

	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, const Grid& grid);

	// выстрелить пачкой частиц
	void emit(const ParticleEmitterProps& props, int count = 1);

private:
	std::vector<Particle> m_pool;
	size_t m_poolIndex; // указатель на следующую свободную частицу
};