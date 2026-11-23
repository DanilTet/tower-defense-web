#include "EnvironmentParticleManager.h"
#include <random>
#include <cmath>
#include <algorithm>
#include "../renderer/SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "../game/world/Grid.h"

static std::random_device s_rd;
static std::mt19937 s_gen(s_rd());
static std::uniform_real_distribution<float> s_dis01(0.0f, 1.0f);

static float randRange(float minVal, float maxVal) {
    return minVal + (maxVal - minVal) * s_dis01(s_gen);
}

EnvironmentParticleManager::EnvironmentParticleManager(size_t maxParticles) {
    m_pool.resize(maxParticles);
    m_poolIndex = maxParticles > 0 ? maxParticles - 1 : 0;
}

void EnvironmentParticleManager::setEmitters(const std::vector<ParticleEmitterConfig>& configs) {
    if (m_emitters.size() == configs.size()) {
        for (size_t i = 0; i < configs.size(); ++i) {
            m_emitters[i].config = configs[i];
            if (configs[i].loopContinuous) {
                m_emitters[i].isBursting = true;
                m_emitters[i].burstRemaining = 9999.0f;
            }
        }
        return;
    }

    m_emitters.clear();
    m_emitters.reserve(configs.size());

    for (const auto& cfg : configs) {
        RuntimeEmitterState state;
        state.config = cfg;
        // Случайный начальный таймер, чтобы эмиттеры не синхронизировались
        state.timer = randRange(0.2f, std::max(0.5f, cfg.periodMin));
        state.burstRemaining = cfg.loopContinuous ? 9999.0f : 0.0f;
        state.spawnAccumulator = 0.0f;
        state.isBursting = cfg.loopContinuous;
        m_emitters.push_back(state);
    }
}

void EnvironmentParticleManager::triggerBurstAt(size_t index) {
    if (index < m_emitters.size()) {
        m_emitters[index].isBursting = true;
        m_emitters[index].burstRemaining = m_emitters[index].config.loopContinuous ? 9999.0f : std::max(0.4f, m_emitters[index].config.burstDuration);
        m_emitters[index].spawnAccumulator = 0.0f;
    }
}

std::vector<ParticleEmitterConfig> EnvironmentParticleManager::getEmitterConfigs() const {
    std::vector<ParticleEmitterConfig> configs;
    configs.reserve(m_emitters.size());
    for (const auto& em : m_emitters) {
        configs.push_back(em.config);
    }
    return configs;
}

void EnvironmentParticleManager::addOrUpdateEmitter(const ParticleEmitterConfig& cfg) {
    for (auto& em : m_emitters) {
        if ((cfg.id > 0 && em.config.id == cfg.id) ||
            glm::distance(em.config.tilePos, cfg.tilePos) < 0.25f) {
            em.config = cfg;
            em.isBursting = true;
            em.burstRemaining = cfg.loopContinuous ? 9999.0f : (cfg.burstDuration > 0.0f ? cfg.burstDuration : 2.2f);
            return;
        }
    }

    RuntimeEmitterState state;
    state.config = cfg;
    state.timer = 0.05f; // быстрый запуск при создании для визуального подтверждения
    state.burstRemaining = cfg.loopContinuous ? 9999.0f : (cfg.burstDuration > 0.0f ? cfg.burstDuration : 2.2f);
    state.spawnAccumulator = 0.0f;
    state.isBursting = true;
    m_emitters.push_back(state);
}

bool EnvironmentParticleManager::removeEmitterAt(int gridX, int gridY) {
    auto it = std::remove_if(m_emitters.begin(), m_emitters.end(),
        [gridX, gridY](const RuntimeEmitterState& em) {
            return static_cast<int>(std::round(em.config.tilePos.x)) == gridX &&
                   static_cast<int>(std::round(em.config.tilePos.y)) == gridY;
        });
    if (it != m_emitters.end()) {
        m_emitters.erase(it, m_emitters.end());
        return true;
    }
    return false;
}

bool EnvironmentParticleManager::rotateEmitterAt(int gridX, int gridY, float deltaDeg) {
    for (auto& em : m_emitters) {
        if (static_cast<int>(std::round(em.config.tilePos.x)) == gridX &&
            static_cast<int>(std::round(em.config.tilePos.y)) == gridY) {
            em.config.angleDeg = std::fmod(em.config.angleDeg + deltaDeg, 360.0f);
            if (em.config.angleDeg < 0.0f) em.config.angleDeg += 360.0f;
            // Провоцируем короткий залп, чтобы сразу было видно направление
            em.isBursting = true;
            em.burstRemaining = 1.0f;
            return true;
        }
    }
    return false;
}

const ParticleEmitterConfig* EnvironmentParticleManager::findEmitterAt(int gridX, int gridY) const {
    for (const auto& em : m_emitters) {
        if (static_cast<int>(std::round(em.config.tilePos.x)) == gridX &&
            static_cast<int>(std::round(em.config.tilePos.y)) == gridY) {
            return &em.config;
        }
    }
    return nullptr;
}

ParticleEmitterConfig* EnvironmentParticleManager::findEmitterAt(int gridX, int gridY) {
    for (auto& em : m_emitters) {
        if (static_cast<int>(std::round(em.config.tilePos.x)) == gridX &&
            static_cast<int>(std::round(em.config.tilePos.y)) == gridY) {
            return &em.config;
        }
    }
    return nullptr;
}

void EnvironmentParticleManager::clear() {
    m_emitters.clear();
    clearParticles();
}

void EnvironmentParticleManager::clearParticles() {
    for (auto& p : m_pool) {
        p.active = false;
    }
}

void EnvironmentParticleManager::emitParticle(const EnvParticle& p) {
    if (m_pool.empty()) return;
    m_pool[m_poolIndex] = p;
    m_pool[m_poolIndex].active = true;

    if (m_poolIndex == 0) {
        m_poolIndex = m_pool.size() - 1;
    } else {
        m_poolIndex--;
    }
}

void EnvironmentParticleManager::triggerEmitterBurst(RuntimeEmitterState& emitter) {
    emitter.isBursting = true;
    emitter.burstRemaining = std::max(0.4f, emitter.config.burstDuration);
}

void EnvironmentParticleManager::update(float dt, const Grid& grid) {
    float cellSize = grid.getCellSize();
    float scaleFactor = cellSize / 64.0f;

    // 1. Обновление эмиттеров
    for (auto& em : m_emitters) {
        glm::vec2 emitterPos = grid.getOffset() + em.config.tilePos * cellSize;
        float baseRad = glm::radians(em.config.angleDeg);
        glm::vec2 mainDir = glm::vec2(std::cos(baseRad), std::sin(baseRad));

        if (em.config.loopContinuous) {
            em.isBursting = true;
            em.burstRemaining = 9999.0f;
        } else {
            if (!em.isBursting) {
                em.timer -= dt;
                if (em.timer <= 0.0f) {
                    triggerEmitterBurst(em);
                }
            }
        }

        if (em.isBursting) {
            if (!em.config.loopContinuous) {
                em.burstRemaining -= dt;
            }
            em.spawnAccumulator += dt;

            float pScale = std::clamp(em.config.particleScale > 0.0f ? em.config.particleScale : 1.0f, 0.4f, 3.0f);
            float cfgSpeed = em.config.speed > 0.0f ? em.config.speed : 180.0f;

            if (em.config.type == "steam_jet") {
                // Высокая плотность пара: каждые 0.016с (60 раз в секунду)
                while (em.spawnAccumulator >= 0.016f) {
                    em.spawnAccumulator -= 0.016f;

                    // Спавним 2-3 микро-частицы пара
                    int steamCount = 2 + static_cast<int>(randRange(0.0f, 1.9f));
                    for (int k = 0; k < steamCount; ++k) {
                        float angleOffset = randRange(-15.0f, 15.0f);
                        float rad = glm::radians(em.config.angleDeg + angleOffset);
                        glm::vec2 dir = glm::vec2(std::cos(rad), std::sin(rad));
                        float speed = (cfgSpeed * randRange(0.80f, 1.25f)) * scaleFactor;

                        EnvParticle p;
                        p.zIndex = em.config.zIndex;
                        p.pos = emitterPos + dir * randRange(2.0f, 8.0f) * scaleFactor;
                        p.vel = dir * speed;
                        p.startColor = glm::vec4(0.96f, 0.98f, 1.0f, randRange(0.65f, 0.85f));
                        p.endColor = glm::vec4(0.85f, 0.88f, 0.92f, 0.0f);
                        p.startSize = randRange(6.0f, 10.0f) * scaleFactor * pScale;
                        p.endSize = randRange(28.0f, 42.0f) * scaleFactor * pScale;
                        p.lifeTime = randRange(0.65f, 1.15f);
                        p.lifeRemaining = p.lifeTime;
                        p.drag = 2.6f;
                        p.gravity = -10.0f; // легкий подъем пара
                        emitParticle(p);
                    }

                    // Редкие капли воды (водяная взвесь)
                    if (s_dis01(s_gen) < 0.12f) {
                        float angleOffset = randRange(-8.0f, 8.0f);
                        float rad = glm::radians(em.config.angleDeg + angleOffset);
                        glm::vec2 dir = glm::vec2(std::cos(rad), std::sin(rad));
                        float speed = (cfgSpeed * randRange(0.45f, 0.85f)) * scaleFactor;

                        EnvParticle p;
                        p.zIndex = em.config.zIndex;
                        p.pos = emitterPos + dir * 6.0f * scaleFactor;
                        p.vel = dir * speed;
                        p.startColor = glm::vec4(0.35f, 0.72f, 1.0f, 0.85f);
                        p.endColor = glm::vec4(0.20f, 0.55f, 0.90f, 0.0f);
                        p.startSize = 4.5f * scaleFactor * pScale;
                        p.endSize = 2.0f * scaleFactor * pScale;
                        p.lifeTime = randRange(1.0f, 1.6f);
                        p.lifeRemaining = p.lifeTime;
                        p.drag = 0.8f;
                        p.gravity = 360.0f; // гравитация тянет вниз
                        emitParticle(p);
                    }
                }
            } else if (em.config.type == "water_drip") {
                while (em.spawnAccumulator >= 0.25f) {
                    em.spawnAccumulator -= 0.25f;
                    float angleOffset = randRange(-12.0f, 12.0f);
                    float rad = glm::radians(em.config.angleDeg + angleOffset);
                    glm::vec2 dir = glm::vec2(std::cos(rad), std::sin(rad));

                    EnvParticle p;
                    p.zIndex = em.config.zIndex;
                    p.pos = emitterPos + dir * 5.0f * scaleFactor;
                    p.vel = dir * (cfgSpeed * randRange(0.20f, 0.40f)) * scaleFactor;
                    p.startColor = glm::vec4(0.30f, 0.75f, 1.0f, 0.9f);
                    p.endColor = glm::vec4(0.20f, 0.60f, 0.95f, 0.0f);
                    p.startSize = 5.0f * scaleFactor * pScale;
                    p.endSize = 2.5f * scaleFactor * pScale;
                    p.lifeTime = randRange(1.2f, 1.8f);
                    p.lifeRemaining = p.lifeTime;
                    p.drag = 0.5f;
                    p.gravity = 380.0f;
                    emitParticle(p);
                }
            } else if (em.config.type == "sparks") {
                while (em.spawnAccumulator >= 0.035f) {
                    em.spawnAccumulator -= 0.035f;
                    for (int k = 0; k < 3; ++k) {
                        float angleOffset = randRange(-28.0f, 28.0f);
                        float rad = glm::radians(em.config.angleDeg + angleOffset);
                        glm::vec2 dir = glm::vec2(std::cos(rad), std::sin(rad));
                        float speed = (cfgSpeed * randRange(0.90f, 1.55f)) * scaleFactor;

                        EnvParticle p;
                        p.zIndex = em.config.zIndex;
                        p.pos = emitterPos;
                        p.vel = dir * speed;
                        p.startColor = glm::vec4(1.0f, 0.95f, 0.40f, 1.0f);
                        p.endColor = glm::vec4(1.0f, 0.20f, 0.0f, 0.0f);
                        p.startSize = randRange(3.5f, 6.0f) * scaleFactor * pScale;
                        p.endSize = 1.5f * scaleFactor * pScale;
                        p.lifeTime = randRange(0.30f, 0.65f);
                        p.lifeRemaining = p.lifeTime;
                        p.drag = 1.2f;
                        p.gravity = 250.0f;
                        emitParticle(p);
                    }
                }
            } else if (em.config.type == "smoke") {
                while (em.spawnAccumulator >= 0.08f) {
                    em.spawnAccumulator -= 0.08f;
                    float angleOffset = randRange(-22.0f, 22.0f);
                    float rad = glm::radians(em.config.angleDeg + angleOffset);
                    glm::vec2 dir = glm::vec2(std::cos(rad), std::sin(rad));

                    EnvParticle p;
                    p.zIndex = em.config.zIndex;
                    p.pos = emitterPos + dir * 6.0f * scaleFactor;
                    p.vel = dir * (cfgSpeed * randRange(0.25f, 0.55f)) * scaleFactor;
                    p.startColor = glm::vec4(0.40f, 0.40f, 0.45f, 0.55f);
                    p.endColor = glm::vec4(0.20f, 0.20f, 0.25f, 0.0f);
                    p.startSize = randRange(8.0f, 14.0f) * scaleFactor * pScale;
                    p.endSize = randRange(35.0f, 55.0f) * scaleFactor * pScale;
                    p.lifeTime = randRange(1.4f, 2.4f);
                    p.lifeRemaining = p.lifeTime;
                    p.drag = 1.4f;
                    p.gravity = -20.0f; // дым медленно поднимается
                    emitParticle(p);
                }
            } else if (em.config.type == "fog") {
                // Туман: крупные, полупрозрачные (alpha 0.05 - 0.15), медленно дрейфующие частицы серого/болотного цвета
                while (em.spawnAccumulator >= 0.22f) {
                    em.spawnAccumulator -= 0.22f;
                    float angleOffset = randRange(-60.0f, 60.0f);
                    float rad = glm::radians(em.config.angleDeg + angleOffset);
                    glm::vec2 dir = glm::vec2(std::cos(rad), std::sin(rad));

                    float scatterRad = glm::radians(randRange(0.0f, 360.0f));
                    glm::vec2 scatterOffset = glm::vec2(std::cos(scatterRad), std::sin(scatterRad)) * randRange(2.0f, 20.0f) * scaleFactor;

                    EnvParticle p;
                    p.zIndex = em.config.zIndex;
                    p.pos = emitterPos + scatterOffset;
                    p.vel = dir * (cfgSpeed * randRange(0.08f, 0.22f)) * scaleFactor;

                    // Серо-болотный дымчатый оттенок
                    glm::vec3 fogTint = glm::vec3(randRange(0.42f, 0.48f), randRange(0.45f, 0.52f), randRange(0.44f, 0.50f));
                    float alphaVal = randRange(0.06f, 0.14f);
                    p.startColor = glm::vec4(fogTint, alphaVal);
                    p.endColor = glm::vec4(fogTint * 0.85f, 0.0f);

                    // Крупный размер
                    p.startSize = randRange(35.0f, 55.0f) * scaleFactor * pScale;
                    p.endSize = randRange(75.0f, 115.0f) * scaleFactor * pScale;

                    // Время жизни 8-12 секунд
                    p.lifeTime = randRange(8.0f, 12.0f);
                    p.lifeRemaining = p.lifeTime;
                    p.drag = 0.6f;
                    p.gravity = -3.0f; // легкое медленное рассеивание
                    p.rotation = randRange(0.0f, 360.0f);
                    p.angularVel = randRange(-8.0f, 8.0f);
                    emitParticle(p);
                }
            }

            if (!em.config.loopContinuous && em.burstRemaining <= 0.0f) {
                em.isBursting = false;
                em.timer = randRange(std::max(0.5f, em.config.periodMin), std::max(em.config.periodMin, em.config.periodMax));
                em.spawnAccumulator = 0.0f;
            }
        }
    }

    // 2. Симуляция частиц в пуле
    for (auto& p : m_pool) {
        if (!p.active) continue;

        p.lifeRemaining -= dt;
        if (p.lifeRemaining <= 0.0f) {
            p.active = false;
            continue;
        }

        if (p.drag > 0.0f) {
            p.vel *= glm::exp(-p.drag * dt);
        }
        if (p.gravity != 0.0f) {
            p.vel.y += p.gravity * dt;
        }

        p.pos += p.vel * dt;
        p.rotation += p.angularVel * dt;
    }
}

void EnvironmentParticleManager::render(SpriteRenderer* renderer, const Texture2D* texture, const Grid& /*grid*/, int minZ, int maxZ) {
    if (!renderer || !texture) return;

    for (const auto& p : m_pool) {
        if (!p.active) continue;
        if (p.zIndex < minZ || p.zIndex > maxZ) continue;

        float lifeFactor = 1.0f - (p.lifeRemaining / p.lifeTime);
        lifeFactor = std::clamp(lifeFactor, 0.0f, 1.0f);

        // Плавное нарастание и затухание альфы
        glm::vec4 currentColor = glm::mix(p.startColor, p.endColor, lifeFactor);
        if (lifeFactor < 0.15f) {
            currentColor.a *= (lifeFactor / 0.15f);
        }
        float currentSize = glm::mix(p.startSize, p.endSize, lifeFactor);

        glm::vec2 drawPos = p.pos - glm::vec2(currentSize * 0.5f);
        renderer->drawSpriteRGBA(texture, drawPos, glm::vec2(currentSize), p.rotation, currentColor);
    }
}

void EnvironmentParticleManager::spawnDemolitionLeaves(glm::vec2 worldCenter, float scaleFactor) {
    float s = std::clamp(scaleFactor, 0.4f, 2.5f);
    // 1. Разлетающиеся зеленые листья (вращение, гравитация, разброс скорости)
    int leafCount = 18 + static_cast<int>(randRange(0.0f, 6.0f));
    for (int i = 0; i < leafCount; ++i) {
        float angleDeg = randRange(0.0f, 360.0f);
        float rad = glm::radians(angleDeg);
        glm::vec2 dir(std::cos(rad), std::sin(rad));
        float speed = randRange(70.0f, 180.0f) * s;

        EnvParticle p;
        p.pos = worldCenter + dir * randRange(2.0f, 10.0f) * s;
        p.vel = dir * speed;
        float gShade = randRange(0.0f, 1.0f);
        if (gShade < 0.35f) {
            p.startColor = glm::vec4(0.20f, 0.72f, 0.22f, 0.95f);
            p.endColor = glm::vec4(0.15f, 0.55f, 0.18f, 0.0f);
        } else if (gShade < 0.70f) {
            p.startColor = glm::vec4(0.38f, 0.85f, 0.25f, 0.95f);
            p.endColor = glm::vec4(0.28f, 0.65f, 0.20f, 0.0f);
        } else {
            p.startColor = glm::vec4(0.12f, 0.48f, 0.15f, 0.95f);
            p.endColor = glm::vec4(0.10f, 0.38f, 0.12f, 0.0f);
        }
        p.startSize = randRange(6.0f, 10.0f) * s;
        p.endSize = randRange(4.0f, 7.0f) * s;
        p.lifeTime = randRange(0.8f, 1.3f);
        p.lifeRemaining = p.lifeTime;
        p.drag = 1.8f;
        p.gravity = randRange(140.0f, 220.0f); // Падают вниз
        p.rotation = randRange(0.0f, 360.0f);
        p.angularVel = randRange(-360.0f, 360.0f); // Вращение
        emitParticle(p);
    }

    // 2. Облачко пыли
    int dustCount = 8;
    for (int i = 0; i < dustCount; ++i) {
        float angleDeg = randRange(0.0f, 360.0f);
        float rad = glm::radians(angleDeg);
        glm::vec2 dir(std::cos(rad), std::sin(rad));
        float speed = randRange(20.0f, 55.0f) * s;

        EnvParticle p;
        p.pos = worldCenter + dir * randRange(1.0f, 8.0f) * s;
        p.vel = dir * speed;
        p.startColor = glm::vec4(0.68f, 0.64f, 0.54f, 0.45f);
        p.endColor = glm::vec4(0.60f, 0.58f, 0.52f, 0.0f);
        p.startSize = randRange(10.0f, 16.0f) * s;
        p.endSize = randRange(32.0f, 48.0f) * s;
        p.lifeTime = randRange(0.6f, 1.0f);
        p.lifeRemaining = p.lifeTime;
        p.drag = 3.2f;
        p.gravity = -8.0f;
        p.rotation = randRange(0.0f, 360.0f);
        p.angularVel = randRange(-60.0f, 60.0f);
        emitParticle(p);
    }
}

void EnvironmentParticleManager::spawnDemolitionSparksGravel(glm::vec2 worldCenter, float scaleFactor) {
    float s = std::clamp(scaleFactor, 0.4f, 2.5f);
    // 1. Искры
    int sparkCount = 14 + static_cast<int>(randRange(0.0f, 5.0f));
    for (int i = 0; i < sparkCount; ++i) {
        float angleDeg = randRange(0.0f, 360.0f);
        float rad = glm::radians(angleDeg);
        glm::vec2 dir(std::cos(rad), std::sin(rad));
        float speed = randRange(150.0f, 300.0f) * s;

        EnvParticle p;
        p.pos = worldCenter;
        p.vel = dir * speed;
        p.startColor = glm::vec4(1.0f, 0.95f, 0.35f, 1.0f);
        p.endColor = glm::vec4(1.0f, 0.35f, 0.05f, 0.0f);
        p.startSize = randRange(3.0f, 5.5f) * s;
        p.endSize = randRange(1.0f, 2.0f) * s;
        p.lifeTime = randRange(0.35f, 0.65f);
        p.lifeRemaining = p.lifeTime;
        p.drag = 1.4f;
        p.gravity = 200.0f;
        emitParticle(p);
    }

    // 2. Щебень и осколки
    int rubbleCount = 12;
    for (int i = 0; i < rubbleCount; ++i) {
        float angleDeg = randRange(0.0f, 360.0f);
        float rad = glm::radians(angleDeg);
        glm::vec2 dir(std::cos(rad), std::sin(rad));
        float speed = randRange(80.0f, 190.0f) * s;

        EnvParticle p;
        p.pos = worldCenter + dir * randRange(1.0f, 6.0f) * s;
        p.vel = dir * speed;
        float rockShade = randRange(0.20f, 0.45f);
        p.startColor = glm::vec4(rockShade, rockShade, rockShade + 0.04f, 1.0f);
        p.endColor = glm::vec4(rockShade * 0.7f, rockShade * 0.7f, rockShade * 0.7f, 0.0f);
        p.startSize = randRange(5.0f, 8.5f) * s;
        p.endSize = randRange(3.5f, 5.5f) * s;
        p.lifeTime = randRange(0.6f, 1.0f);
        p.lifeRemaining = p.lifeTime;
        p.drag = 0.9f;
        p.gravity = 350.0f;
        p.rotation = randRange(0.0f, 360.0f);
        p.angularVel = randRange(-450.0f, 450.0f);
        emitParticle(p);
    }

    // 3. Серая каменная пыль
    int dustCount = 6;
    for (int i = 0; i < dustCount; ++i) {
        float angleDeg = randRange(0.0f, 360.0f);
        float rad = glm::radians(angleDeg);
        glm::vec2 dir(std::cos(rad), std::sin(rad));
        float speed = randRange(25.0f, 65.0f) * s;

        EnvParticle p;
        p.pos = worldCenter + dir * randRange(1.0f, 6.0f) * s;
        p.vel = dir * speed;
        p.startColor = glm::vec4(0.48f, 0.46f, 0.44f, 0.50f);
        p.endColor = glm::vec4(0.35f, 0.35f, 0.35f, 0.0f);
        p.startSize = randRange(10.0f, 16.0f) * s;
        p.endSize = randRange(34.0f, 50.0f) * s;
        p.lifeTime = randRange(0.5f, 0.85f);
        p.lifeRemaining = p.lifeTime;
        p.drag = 3.0f;
        p.gravity = -5.0f;
        emitParticle(p);
    }
}

void EnvironmentParticleManager::spawnDemolitionSplash(glm::vec2 worldCenter, float scaleFactor) {
    float s = std::clamp(scaleFactor, 0.4f, 2.5f);
    int splashCount = 16;
    for (int i = 0; i < splashCount; ++i) {
        float angleDeg = randRange(0.0f, 360.0f);
        float rad = glm::radians(angleDeg);
        glm::vec2 dir(std::cos(rad), std::sin(rad));
        float speed = randRange(80.0f, 180.0f) * s;

        EnvParticle p;
        p.pos = worldCenter + dir * randRange(2.0f, 8.0f) * s;
        p.vel = dir * speed;
        p.startColor = glm::vec4(0.35f, 0.72f, 0.95f, 0.90f);
        p.endColor = glm::vec4(0.20f, 0.55f, 0.85f, 0.0f);
        p.startSize = randRange(4.5f, 7.5f) * s;
        p.endSize = randRange(2.0f, 3.5f) * s;
        p.lifeTime = randRange(0.5f, 0.9f);
        p.lifeRemaining = p.lifeTime;
        p.drag = 1.0f;
        p.gravity = 300.0f;
        emitParticle(p);
    }
}

