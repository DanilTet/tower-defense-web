#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../renderer/SpriteRenderer.h"
// один кадр
struct AnimationFrame
{
	SpriteUV uv; // координаты вырезки из атласа
	float duration; // длительность кадра в секундах
};

// анимация уже
struct AnimationClip {
	std::vector<AnimationFrame> frames;
	bool loop = true; // зациклить ли анимку?
};

class Animator {
private:
    std::unordered_map<std::string, AnimationClip> m_animations;
    std::string m_currentAnimation;

    size_t m_currentFrameIndex = 0;
    float m_timer = 0.0f;

public:
    // добавить новую анимацию в словарь
    void addAnimation(const std::string& name, const AnimationClip& clip) {
        m_animations[name] = clip;
    }

    // запустить анимацию
    void play(const std::string& name) {
        // если анимация уже идет то сбрасываем ее
        if (m_currentAnimation == name) return;

        m_currentAnimation = name;
        m_currentFrameIndex = 0;
        m_timer = 0.0f;
    }

    // обновление логики времени
    void update(float dt) {
        if (m_currentAnimation.empty() || m_animations.find(m_currentAnimation) == m_animations.end()) return;

        auto& clip = m_animations[m_currentAnimation];
        if (clip.frames.empty()) return;

        m_timer += dt;

        float frameDuration = clip.frames[m_currentFrameIndex].duration;

        while (m_timer >= frameDuration) {
            m_timer -= frameDuration;
            m_currentFrameIndex++;

            if (m_currentFrameIndex >= clip.frames.size()) {
                if (clip.loop) {
                    m_currentFrameIndex = 0; // начинаем сначала
                }
                else {
                    m_currentFrameIndex = clip.frames.size() - 1; // застываем на последнем кадре
                }
            }
            frameDuration = clip.frames[m_currentFrameIndex].duration;
        }
    }

    // получить текущие UV координаты для SpriteRenderer
    SpriteUV getCurrentUV() const {
        if (m_currentAnimation.empty() || m_animations.find(m_currentAnimation) == m_animations.end()) {
            return SpriteUV(); // если анимки нету
        }
        return m_animations.at(m_currentAnimation).frames[m_currentFrameIndex].uv;
    }
};