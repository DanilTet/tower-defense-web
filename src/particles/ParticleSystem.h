#pragma once

#include <cstdint>
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
};


class ParticleSystem {
public:
	// жёсткий лимит активных частиц с предвыделением памяти
	explicit ParticleSystem(size_t maxParticles = 800);

	void update(float dt);

	void render(SpriteRenderer* renderer, const Texture2D* texture, const Grid& grid);

	// выстрелить пачкой частиц
	void emit(const ParticleEmitterProps& props, int count = 1, bool isTrail = false);

	void setTimeScale(float timeScale);
	float getTimeScale() const { return m_timeScale; }
	size_t getActiveCount() const { return m_particles.size(); }
	size_t getMaxParticles() const { return m_maxParticles; }
	size_t getTrailSoftCap() const { return m_trailSoftCap; }

private:
	float nextRandomSigned();

	std::vector<Particle> m_particles;
	size_t m_maxParticles;
	size_t m_trailSoftCap;
	float m_timeScale = 1.0f;
	uint32_t m_rngState = 0x9E3779B9u;
};