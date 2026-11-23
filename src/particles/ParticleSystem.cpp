#include "ParticleSystem.h"
#include <algorithm>
#include <cmath>
#include "../renderer/SpriteRenderer.h" 
#include "../textures/Texture2D.h"
#include "../game/world/Grid.h"

ParticleSystem::ParticleSystem(size_t maxParticles) {
    if (maxParticles == 0) {
        maxParticles = 1;
    }
    m_maxParticles = maxParticles;
    m_trailSoftCap = m_maxParticles / 2;
    m_particles.reserve(m_maxParticles);
}

void ParticleSystem::setTimeScale(float timeScale) {
    if (timeScale < 1.0f) {
        m_timeScale = 1.0f;
    }
    else {
        m_timeScale = timeScale;
    }
}

float ParticleSystem::nextRandomSigned() {
    uint32_t x = m_rngState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    m_rngState = x;
    // Нормализация в диапазон [-1.0f, 1.0f]
    float normalized01 = static_cast<float>(x & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu);
    return normalized01 * 2.0f - 1.0f;
}

void ParticleSystem::update(float dt) {
    if (m_particles.empty()) {
        return;
    }

    // Считаем коэффициент затухания один раз за шаг для всех активных частиц
    const float dragFactor = std::exp(-6.0f * dt);

    size_t i = 0;
    while (i < m_particles.size()) {
        Particle& p = m_particles[i];
        p.lifeRemaining -= dt;
        if (p.lifeRemaining <= 0.0f) {
            if (i + 1 < m_particles.size()) {
                m_particles[i] = m_particles.back();
            }
            m_particles.pop_back();
            continue;
        }

        p.velocity *= dragFactor;
        p.position += p.velocity * dt;
        ++i;
    }
}

void ParticleSystem::render(SpriteRenderer* renderer, const Texture2D* texture, const Grid& grid) {
    if (!renderer || !texture || m_particles.empty()) {
        return;
    }

    float scaleFactor = grid.getCellSize() / 64.0f;
    for (const Particle& p : m_particles) {
        float lifeFactor = std::clamp(1.0f - (p.lifeRemaining / p.lifeTime), 0.0f, 1.0f);

        glm::vec3 currentColor = glm::mix(p.startColor, p.endColor, lifeFactor);
        float currentSize = glm::mix(p.startSize, p.endSize, lifeFactor) * scaleFactor;

        glm::vec2 drawPos = p.position - glm::vec2(currentSize * 0.5f);
        renderer->drawSprite(texture, drawPos, glm::vec2(currentSize), 0.0f, currentColor);
    }
}

void ParticleSystem::emit(const ParticleEmitterProps& props, int count, bool isTrail) {
    if (count <= 0) {
        return;
    }

    int effectiveCount = count;
    size_t budget = 0;

    if (isTrail) {
        if (m_particles.size() >= m_trailSoftCap) {
            return;
        }
        budget = m_trailSoftCap - m_particles.size();
    }
    else {
        if (m_particles.size() >= m_maxParticles) {
            return;
        }
        if (m_timeScale >= 3.5f) {
            effectiveCount = std::max(1, (count + 2) / 3);
        }
        else if (m_timeScale >= 1.5f) {
            effectiveCount = std::max(1, (count + 1) / 2);
        }
        if (m_particles.size() > (m_maxParticles * 3) / 4) {
            effectiveCount = std::min(effectiveCount, 2);
        }
        budget = m_maxParticles - m_particles.size();
    }

    if (static_cast<size_t>(effectiveCount) > budget) {
        effectiveCount = static_cast<int>(budget);
    }
    if (effectiveCount <= 0) {
        return;
    }

    float dirLen = glm::length(props.velocityDir);
    bool hasDir = (dirLen > 0.0001f);
    glm::vec2 baseDir = glm::vec2(0.0f, -1.0f);
    float maxAngle = 0.0f;
    if (hasDir) {
        baseDir = props.velocityDir / dirLen;
        maxAngle = props.velocityVariation / dirLen;
    }

    for (int i = 0; i < effectiveCount; ++i) {
        m_particles.emplace_back();
        Particle& p = m_particles.back();
        p.position = props.position;

        if (hasDir) {
            float speed = dirLen + nextRandomSigned() * props.velocityVariation * 0.5f;
            float angle = nextRandomSigned() * maxAngle;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            glm::vec2 rotatedDir = glm::vec2(
                baseDir.x * cosA - baseDir.y * sinA,
                baseDir.x * sinA + baseDir.y * cosA
            );
            p.velocity = rotatedDir * speed;
        }
        else {
            float speed = std::abs(nextRandomSigned() * props.velocityVariation);
            float angle = nextRandomSigned() * 3.14159265f;
            p.velocity = glm::vec2(std::cos(angle), std::sin(angle)) * speed;
        }

        p.startColor = props.startColor;
        p.endColor = props.endColor;
        p.startSize = props.startSize;
        p.endSize = props.endSize;

        float life = props.lifeTime * (1.0f + nextRandomSigned() * 0.2f);
        if (life < 0.05f) {
            life = 0.05f;
        }
        p.lifeTime = life;
        p.lifeRemaining = life;
    }
}
