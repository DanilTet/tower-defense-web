#include "ParticleSystem.h"
#include <random>
#include "../renderer/SpriteRenderer.h" 
#include "../textures/Texture2D.h"
#include "../game/world/Grid.h"

// генератор рандомных чисел для красивого разлета
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

ParticleSystem::ParticleSystem(size_t maxParticles) {
    m_pool.resize(maxParticles);
    m_poolIndex = maxParticles - 1;
}

void ParticleSystem::update(float dt) {
    for (auto& p : m_pool) {
        if (!p.active) continue;

        // отнимаем время жизни
        p.lifeRemaining -= dt;
        if (p.lifeRemaining <= 0.0f) {
            p.active = false; // если частица все то камбекаем ее в пулл
            continue;
        }

        // Применяем торможение (затухание скорости)
        p.velocity *= glm::exp(-6.0f * dt);

        // двигаем гада
        p.position += p.velocity * dt;
    }
}

void ParticleSystem::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, const Grid& grid) {
    float scaleFactor = grid.getCellSize() / 64.0f; // масштаб
    for (auto& p : m_pool) {
        if (!p.active) continue;

        // вычисляем процент жизни от 0.0 это только родилась до 1.0 умерла
        float lifeFactor = 1.0f - (p.lifeRemaining / p.lifeTime);

        // интерполяция цвета и размера
        glm::vec3 currentColor = glm::mix(p.startColor, p.endColor, lifeFactor);
        float currentSize = glm::mix(p.startSize, p.endSize, lifeFactor) * scaleFactor;

        // рисуем отцентрованную частицу
        glm::vec2 drawPos = p.position - glm::vec2(currentSize / 2.0f);
        renderer->drawSprite(texture, drawPos, glm::vec2(currentSize), 0.0f, currentColor);
    }
}

void ParticleSystem::emit(const ParticleEmitterProps& props, int count) {
    for (int i = 0; i < count; ++i) {
        Particle& p = m_pool[m_poolIndex];
        p.active = true;
        p.position = props.position;

        // генерируем случайный вектор разлета
        glm::vec2 baseDir = glm::vec2(0.0f, -1.0f);

        // если вектор не нулевой то нормализуем его для ровного круга взрыва
        if (glm::length(props.velocityDir) > 0.0f) {
            baseDir = glm::normalize(props.velocityDir);
            float speed = glm::length(props.velocityDir) + dis(gen) * props.velocityVariation * 0.5f;
            // вычисляем угол конуса в радианах на основе вариации
            float maxAngle = props.velocityVariation / glm::length(props.velocityDir);
            float angle = dis(gen) * maxAngle;
            
            float cosA = cos(angle);
            float sinA = sin(angle);
            glm::vec2 rotatedDir = glm::vec2(
                baseDir.x * cosA - baseDir.y * sinA,
                baseDir.x * sinA + baseDir.y * cosA
            );
            p.velocity = rotatedDir * speed;
        } else {
            float speed = abs(dis(gen) * props.velocityVariation);
            float angle = dis(gen) * 3.14159265f;
            p.velocity = glm::vec2(cos(angle), sin(angle)) * speed;
        }

        // задаем визуал
        p.startColor = props.startColor;
        p.endColor = props.endColor;
        p.startSize = props.startSize;
        p.endSize = props.endSize;

        // добавляем микро-рандом к времени жизни чтобы они не исчезали синхронно
        float lifeRand = (dis(gen) * 0.2f); // +/- 20%
        p.lifeTime = props.lifeTime + lifeRand;
        p.lifeRemaining = p.lifeTime;

        // идем по кругу пула
        if (m_poolIndex == 0) {
            m_poolIndex = m_pool.size() - 1;
        }
        else {
            m_poolIndex = m_poolIndex - 1;
        }
    }
}
