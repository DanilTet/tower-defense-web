#include "DecorationRenderer.h"
#include "SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "../resources/ResourceManager.h"
#include "../game/world/Grid.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace {

void drawPart(
    SpriteRenderer* renderer,
    const Texture2D* texture,
    glm::vec2 decorPos,
    float decorRot,
    glm::vec2 decorScale,
    glm::vec2 localOffset,
    glm::vec2 localSize,
    float localAngle,
    glm::vec4 color
) {
    if (!renderer || !texture) return;

    float rad = glm::radians(decorRot);
    float cosR = std::cos(rad);
    float sinR = std::sin(rad);

    glm::vec2 scaledOffset = localOffset * decorScale;
    glm::vec2 rotOffset(
        scaledOffset.x * cosR - scaledOffset.y * sinR,
        scaledOffset.x * sinR + scaledOffset.y * cosR
    );

    glm::vec2 finalCenter = decorPos + rotOffset;
    glm::vec2 finalSize = localSize * decorScale;
    glm::vec2 drawPos = finalCenter - finalSize * 0.5f;

    renderer->drawSpriteRGBA(texture, drawPos, finalSize, localAngle + decorRot, color);
}

void drawBush(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Темно-зеленый базис кроны (круглые перекрывающиеся объемы для зубчатого пушистого края)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 44.0f, 42.0f }, 0.0f, glm::vec4(0.09f, 0.28f, 0.12f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -14.0f, -11.0f }, { 24.0f, 23.0f }, 15.0f, glm::vec4(0.10f, 0.30f, 0.13f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 13.0f, -12.0f }, { 23.0f, 22.0f }, -20.0f, glm::vec4(0.11f, 0.32f, 0.14f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 14.0f, 10.0f }, { 25.0f, 24.0f }, 25.0f, glm::vec4(0.09f, 0.27f, 0.11f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -13.0f, 11.0f }, { 24.0f, 23.0f }, -15.0f, glm::vec4(0.08f, 0.26f, 0.11f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -17.0f, 0.0f }, { 21.0f, 21.0f }, 5.0f, glm::vec4(0.10f, 0.30f, 0.13f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 17.0f, -1.0f }, { 21.0f, 21.0f }, -10.0f, glm::vec4(0.11f, 0.32f, 0.14f, a));

    // 2. Средне-зеленый объем листвы
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -5.0f, -5.0f }, { 28.0f, 26.0f }, 10.0f, glm::vec4(0.18f, 0.52f, 0.22f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 6.0f, -3.0f }, { 27.0f, 26.0f }, -15.0f, glm::vec4(0.20f, 0.56f, 0.24f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -2.0f, 6.0f }, { 26.0f, 24.0f }, 20.0f, glm::vec4(0.17f, 0.50f, 0.21f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 5.0f, 5.0f }, { 24.0f, 23.0f }, -5.0f, glm::vec4(0.19f, 0.54f, 0.23f, a));

    // 3. Салатовые блики по центру для объемного купола сверху
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -2.0f, -3.0f }, { 19.0f, 18.0f }, 5.0f, glm::vec4(0.34f, 0.74f, 0.30f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 4.0f, -1.0f }, { 16.0f, 15.0f }, -10.0f, glm::vec4(0.38f, 0.78f, 0.32f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -2.0f }, { 10.0f, 9.0f }, 0.0f, glm::vec4(0.48f, 0.88f, 0.38f, 0.95f * a));
}

struct GrassBladeDesc {
    float x;
    float y;
    float height;
    float width;
    float phase;
};

void drawGrassTuft(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);
    float time = static_cast<float>(glfwGetTime());

    // 1. Небольшие мягкие корневые тени под скоплениями травинок
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 4.0f }, { 36.0f, 14.0f }, 0.0f, glm::vec4(0.05f, 0.10f, 0.05f, 0.25f * a));

    // Набор вертикальных травинок (18-22 шт.), хаотично распределенных по пятну ~40-50px
    static const GrassBladeDesc s_blades[] = {
        { -19.0f,   4.0f, 13.0f, 1.8f, 0.1f },
        { -15.0f,   8.0f, 16.0f, 1.9f, 0.8f },
        { -12.0f,  -2.0f, 18.0f, 1.8f, 1.4f },
        { -10.0f,   6.0f, 14.0f, 1.7f, 2.1f },
        {  -7.0f,   1.0f, 21.0f, 2.0f, 0.5f },
        {  -5.0f,  -6.0f, 15.0f, 1.8f, 2.9f },
        {  -3.0f,   8.0f, 19.0f, 1.9f, 1.1f },
        {  -1.0f,   2.0f, 22.0f, 2.0f, 0.2f },
        {   1.0f,  -4.0f, 17.0f, 1.8f, 1.7f },
        {   3.0f,   7.0f, 20.0f, 1.9f, 2.5f },
        {   6.0f,   0.0f, 23.0f, 2.0f, 0.9f },
        {   8.0f,  -5.0f, 14.0f, 1.7f, 3.2f },
        {  11.0f,   6.0f, 18.0f, 1.9f, 1.5f },
        {  13.0f,  -2.0f, 21.0f, 2.0f, 0.4f },
        {  15.0f,   5.0f, 15.0f, 1.8f, 2.3f },
        {  18.0f,  -1.0f, 17.0f, 1.8f, 1.9f },
        {  20.0f,   6.0f, 12.0f, 1.7f, 0.7f },
        {  -9.0f,  -8.0f, 13.0f, 1.7f, 1.8f },
        {   0.0f, -10.0f, 14.0f, 1.8f, 0.6f },
        {   9.0f,  -9.0f, 13.0f, 1.7f, 2.7f }
    };

    for (const auto& b : s_blades) {
        // Колыхание верхушки от ветра (основание зафиксировано на земле)
        float sway = std::sin(time * 2.2f + b.x * 0.05f + b.phase) * (b.height * 0.15f);
        float tiltAngleDeg = glm::degrees(std::atan2(sway, b.height));

        // Центр основного тела травинки
        glm::vec2 bodyCenter(b.x + sway * 0.5f, b.y - b.height * 0.5f);
        drawPart(renderer, tex, d.worldPos, d.rotation, sVec,
                 bodyCenter, { b.width, b.height }, tiltAngleDeg,
                 glm::vec4(0.22f, 0.68f, 0.28f, 0.70f * a));

        // Светлый кончик травинки (верхние 30% высоты)
        float tipH = b.height * 0.32f;
        glm::vec2 tipCenter(b.x + sway * 0.84f, b.y - b.height + tipH * 0.5f);
        drawPart(renderer, tex, d.worldPos, d.rotation, sVec,
                 tipCenter, { b.width * 0.9f, tipH }, tiltAngleDeg,
                 glm::vec4(0.40f, 0.86f, 0.35f, 0.78f * a));
    }
}

void drawGrassField(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);
    float time = static_cast<float>(glfwGetTime());

    // Плотная россыпь из 48 тонких вертикальных травинок по площади тайла 64x64px
    // Базовая высота травинок уменьшена на 35% (9.0f - 14.0f вместо 14.0f - 22.0f)
    // чтобы они не вылезали за пределы клетки и аккуратно заполняли внутренность тайла.
    static const GrassBladeDesc s_fieldBlades[48] = {
        { -27.0f, -22.0f, 10.0f, 1.4f, 0.2f },
        { -20.0f, -24.0f, 11.5f, 1.5f, 1.1f },
        { -12.0f, -23.0f,  9.5f, 1.3f, 2.4f },
        {  -4.0f, -25.0f, 12.0f, 1.5f, 0.8f },
        {   5.0f, -23.0f, 11.0f, 1.4f, 1.9f },
        {  13.0f, -24.0f, 10.0f, 1.3f, 3.1f },
        {  21.0f, -22.0f, 12.0f, 1.5f, 0.5f },
        {  28.0f, -23.0f,  9.5f, 1.4f, 2.0f },

        { -28.0f, -14.0f, 11.0f, 1.4f, 1.5f },
        { -22.0f, -12.0f, 13.0f, 1.6f, 0.3f },
        { -14.0f, -15.0f, 10.0f, 1.3f, 2.8f },
        {  -6.0f, -13.0f, 13.5f, 1.5f, 1.0f },
        {   2.0f, -14.0f, 11.5f, 1.4f, 2.3f },
        {  11.0f, -12.0f, 13.0f, 1.6f, 0.7f },
        {  19.0f, -15.0f, 11.0f, 1.4f, 2.7f },
        {  27.0f, -13.0f, 12.0f, 1.5f, 1.2f },

        { -26.0f,  -4.0f, 12.0f, 1.5f, 0.6f },
        { -18.0f,  -6.0f, 10.0f, 1.3f, 2.2f },
        { -10.0f,  -3.0f, 14.0f, 1.6f, 1.4f },
        {  -2.0f,  -5.0f, 11.5f, 1.4f, 0.1f },
        {   7.0f,  -4.0f, 13.5f, 1.6f, 2.9f },
        {  15.0f,  -6.0f, 11.0f, 1.4f, 1.8f },
        {  23.0f,  -3.0f, 13.0f, 1.5f, 0.4f },
        {  29.0f,  -5.0f, 10.0f, 1.3f, 2.5f },

        { -27.0f,   5.0f, 11.0f, 1.4f, 2.0f },
        { -21.0f,   3.0f, 13.5f, 1.6f, 0.9f },
        { -13.0f,   6.0f, 11.5f, 1.4f, 3.2f },
        {  -5.0f,   4.0f, 14.5f, 1.6f, 1.6f },
        {   3.0f,   5.0f, 12.0f, 1.5f, 0.2f },
        {  12.0f,   3.0f, 14.0f, 1.6f, 2.6f },
        {  20.0f,   6.0f, 11.0f, 1.4f, 1.1f },
        {  28.0f,   4.0f, 13.0f, 1.5f, 3.0f },

        { -28.0f,  15.0f, 13.0f, 1.5f, 0.7f },
        { -19.0f,  17.0f, 11.0f, 1.4f, 2.5f },
        { -11.0f,  14.0f, 14.0f, 1.6f, 1.3f },
        {  -3.0f,  16.0f, 11.5f, 1.4f, 3.0f },
        {   6.0f,  15.0f, 13.5f, 1.6f, 0.5f },
        {  14.0f,  17.0f, 11.0f, 1.4f, 2.1f },
        {  22.0f,  14.0f, 13.0f, 1.5f, 1.7f },
        {  27.0f,  16.0f, 10.0f, 1.3f, 0.8f },

        { -26.0f,  26.0f, 11.0f, 1.4f, 1.4f },
        { -18.0f,  24.0f, 12.0f, 1.5f, 2.8f },
        { -10.0f,  27.0f, 10.0f, 1.3f, 0.4f },
        {  -1.0f,  25.0f, 13.0f, 1.5f, 1.9f },
        {   8.0f,  26.0f, 11.0f, 1.4f, 3.3f },
        {  16.0f,  24.0f, 13.5f, 1.6f, 1.0f },
        {  24.0f,  27.0f, 10.0f, 1.3f, 2.4f },
        {  29.0f,  25.0f, 11.5f, 1.4f, 0.6f }
    };

    for (const auto& b : s_fieldBlades) {
        float sway = std::sin(time * 2.2f + b.x * 0.05f + b.phase) * (b.height * 0.12f);
        float tiltAngleDeg = glm::degrees(std::atan2(sway, b.height));

        // Основание травинки: более плотное и насыщенное
        glm::vec2 bodyCenter(b.x + sway * 0.5f, b.y - b.height * 0.5f);
        drawPart(renderer, tex, d.worldPos, d.rotation, sVec,
                 bodyCenter, { b.width, b.height }, tiltAngleDeg,
                 glm::vec4(0.20f, 0.62f, 0.24f, 0.85f * a));

        // Мягкий светлый кончик (верхние 30% высоты)
        float tipH = b.height * 0.32f;
        glm::vec2 tipCenter(b.x + sway * 0.84f, b.y - b.height + tipH * 0.5f);
        drawPart(renderer, tex, d.worldPos, d.rotation, sVec,
                 tipCenter, { b.width * 0.85f, tipH }, tiltAngleDeg,
                 glm::vec4(0.44f, 0.90f, 0.38f, 0.92f * a));
    }
}

void drawFlower(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Тень
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.5f, 2.0f }, { 24.0f, 20.0f }, 0.0f, glm::vec4(0.08f, 0.12f, 0.08f, 0.35f * a));
    // 2. Листики под цветками
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -8.0f, 4.0f }, { 12.0f, 5.5f }, -30.0f, glm::vec4(0.20f, 0.55f, 0.18f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 8.0f, 4.0f }, { 12.0f, 5.5f }, 30.0f, glm::vec4(0.25f, 0.62f, 0.22f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -9.0f }, { 5.5f, 12.0f }, 0.0f, glm::vec4(0.22f, 0.58f, 0.20f, a));
    // 3. Лепестки главного цветка (вид сверху, распустившийся бутон)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 0.0f }, { 16.0f, 8.0f }, 0.0f, glm::vec4(0.95f, 0.25f, 0.38f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 0.0f }, { 8.0f, 16.0f }, 0.0f, glm::vec4(0.95f, 0.25f, 0.38f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 0.0f }, { 14.0f, 7.0f }, 45.0f, glm::vec4(0.98f, 0.35f, 0.48f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 0.0f }, { 7.0f, 14.0f }, 45.0f, glm::vec4(0.98f, 0.35f, 0.48f, a));
    // 4. Золотистая сердцевина
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 0.0f }, { 6.5f, 6.5f }, 0.0f, glm::vec4(1.0f, 0.88f, 0.15f, a));
    // 5. Маленький синий цветок рядом
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 11.0f, -4.0f }, { 10.0f, 5.0f }, 25.0f, glm::vec4(0.35f, 0.65f, 1.0f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 11.0f, -4.0f }, { 5.0f, 10.0f }, 25.0f, glm::vec4(0.35f, 0.65f, 1.0f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 11.0f, -4.0f }, { 4.0f, 4.0f }, 0.0f, glm::vec4(1.0f, 0.90f, 0.20f, a));
}

void drawStone(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Тень
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 2.5f, 3.0f }, { 34.0f, 26.0f }, 0.0f, glm::vec4(0.06f, 0.07f, 0.09f, 0.40f * a));
    // 2. Темная грань (низ-право)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.5f, 1.5f }, { 30.0f, 23.0f }, 5.0f, glm::vec4(0.22f, 0.23f, 0.26f, a));
    // 3. Основное тело камня
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, -1.0f }, { 27.0f, 20.0f }, -3.0f, glm::vec4(0.38f, 0.40f, 0.44f, a));
    // 4. Освещенная грань скола (верх-лево)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -4.0f, -4.0f }, { 18.0f, 13.0f }, -8.0f, glm::vec4(0.52f, 0.54f, 0.58f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -5.0f, -6.0f }, { 11.0f, 7.0f }, -12.0f, glm::vec4(0.68f, 0.70f, 0.74f, a));
    // 5. Мелкий камешек рядом
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -14.0f, 8.0f }, { 9.0f, 7.0f }, 10.0f, glm::vec4(0.40f, 0.42f, 0.46f, a));
}

void drawHelmet(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Тень
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.5f, 2.0f }, { 28.0f, 24.0f }, 0.0f, glm::vec4(0.06f, 0.06f, 0.08f, 0.35f * a));
    // 2. Поля каски (овальный ободок)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 28.0f, 24.0f }, 0.0f, glm::vec4(0.82f, 0.58f, 0.05f, a));
    // 3. Купол каски
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 22.0f, 19.0f }, 0.0f, glm::vec4(0.98f, 0.78f, 0.08f, a));
    // 4. Продольное ребро жесткости (сверху вдоль каски)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 5.0f, 20.0f }, 0.0f, glm::vec4(1.0f, 0.90f, 0.28f, a));
    // 5. Крепление шахтерского налобного фонаря
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -11.0f }, { 6.0f, 5.0f }, 0.0f, glm::vec4(0.24f, 0.26f, 0.30f, a));
    // 6. Линза фонаря
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -13.0f }, { 4.0f, 3.0f }, 0.0f, glm::vec4(0.90f, 0.96f, 1.0f, a));
    // 7. Световой конус фонаря
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -20.0f }, { 16.0f, 12.0f }, 0.0f, glm::vec4(1.0f, 0.95f, 0.55f, 0.30f * a));
}

void drawPickaxe(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Тень
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.5f, 2.0f }, { 3.5f, 42.0f }, 0.0f, glm::vec4(0.06f, 0.06f, 0.08f, 0.35f * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.5f, -12.0f }, { 44.0f, 5.0f }, 0.0f, glm::vec4(0.06f, 0.06f, 0.08f, 0.35f * a));

    // 2. Прямая деревянная рукоять
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 4.0f }, { 3.5f, 40.0f }, 0.0f, glm::vec4(0.58f, 0.40f, 0.24f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -0.7f, 4.0f }, { 1.2f, 38.0f }, 0.0f, glm::vec4(0.72f, 0.52f, 0.32f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 23.0f }, { 5.0f, 3.5f }, 0.0f, glm::vec4(0.42f, 0.28f, 0.16f, a));

    // 3. Стальная проушина/обух на конце рукояти
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -14.0f }, { 7.0f, 8.0f }, 0.0f, glm::vec4(0.24f, 0.25f, 0.28f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -14.0f }, { 2.5f, 2.5f }, 0.0f, glm::vec4(0.38f, 0.28f, 0.18f, a));

    // 4. Левое лезвие (заостренный носик-клюв)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -7.0f, -14.0f }, { 8.0f, 5.0f }, 0.0f, glm::vec4(0.32f, 0.34f, 0.38f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -14.0f, -14.0f }, { 8.0f, 3.8f }, 0.0f, glm::vec4(0.42f, 0.44f, 0.48f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -20.0f, -14.0f }, { 5.0f, 2.2f }, 0.0f, glm::vec4(0.68f, 0.72f, 0.78f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -23.5f, -14.0f }, { 2.5f, 1.2f }, 0.0f, glm::vec4(0.88f, 0.90f, 0.95f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -13.0f, -15.5f }, { 16.0f, 1.0f }, 0.0f, glm::vec4(0.75f, 0.78f, 0.85f, a));

    // 5. Правое лезвие (прямой боек / обух)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 7.0f, -14.0f }, { 8.0f, 5.5f }, 0.0f, glm::vec4(0.32f, 0.34f, 0.38f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 13.0f, -14.0f }, { 6.0f, 5.0f }, 0.0f, glm::vec4(0.44f, 0.46f, 0.50f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 17.5f, -14.0f }, { 3.5f, 5.5f }, 0.0f, glm::vec4(0.65f, 0.68f, 0.74f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 19.5f, -14.0f }, { 1.5f, 5.0f }, 0.0f, glm::vec4(0.82f, 0.85f, 0.90f, a));
}

void drawPuddle(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);
    float op = std::clamp(d.opacity, 0.10f, 1.0f);
    float baseWaterAlpha = 0.45f * (op / 0.5f);

    // 1. Мягкая влажная кайма грунта/асфальта вокруг лужи
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 52.0f, 32.0f }, 0.0f, glm::vec4(0.08f, 0.12f, 0.14f, 0.18f * baseWaterAlpha * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 3.0f, 1.0f }, { 44.0f, 28.0f }, 5.0f, glm::vec4(0.08f, 0.12f, 0.14f, 0.15f * baseWaterAlpha * a));

    // 2. Тело лужи (полупрозрачная вода 0.15, 0.22, 0.28, baseWaterAlpha)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 46.0f, 26.0f }, 0.0f, glm::vec4(0.15f, 0.22f, 0.28f, baseWaterAlpha * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 2.0f, 1.0f }, { 36.0f, 22.0f }, 4.0f, glm::vec4(0.14f, 0.21f, 0.27f, 0.85f * baseWaterAlpha * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -3.0f, -1.0f }, { 28.0f, 18.0f }, -6.0f, glm::vec4(0.13f, 0.20f, 0.26f, 0.75f * baseWaterAlpha * a));

    // 3. Более глубокая зона в центре
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 1.0f }, { 24.0f, 13.0f }, 0.0f, glm::vec4(0.11f, 0.18f, 0.23f, 0.60f * baseWaterAlpha * a));

    // 4. Тонкий мягкий блик отражения неба по верхнему краю лужи
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -8.5f }, { 28.0f, 2.2f }, -2.0f, glm::vec4(0.68f, 0.84f, 0.96f, 0.28f * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -6.0f, -7.5f }, { 14.0f, 1.6f }, -5.0f, glm::vec4(0.78f, 0.90f, 1.0f, 0.32f * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 8.0f, -4.0f }, { 12.0f, 1.8f }, 8.0f, glm::vec4(0.60f, 0.78f, 0.92f, 0.22f * a));

    // 5. Тонкая светлая рябь
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -4.0f, 2.0f }, { 15.0f, 1.4f }, -3.0f, glm::vec4(0.65f, 0.82f, 0.95f, 0.20f * a));
}

void drawCrack(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);
    float crackAlpha = std::clamp(d.opacity, 0.10f, 1.0f) * a;

    // 1. Разлом
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 40.0f, 3.5f }, 12.0f, glm::vec4(0.04f, 0.04f, 0.06f, 0.95f * crackAlpha));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -12.0f, -3.5f }, { 16.0f, 2.5f }, -35.0f, glm::vec4(0.05f, 0.05f, 0.07f, 0.90f * crackAlpha));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 13.0f, 3.5f }, { 17.0f, 2.5f }, 45.0f, glm::vec4(0.05f, 0.05f, 0.07f, 0.90f * crackAlpha));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -2.0f, 2.5f }, { 12.0f, 2.0f }, -18.0f, glm::vec4(0.06f, 0.06f, 0.08f, 0.80f * crackAlpha));
    // 2. Сколы краев
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -2.5f }, { 34.0f, 1.5f }, 12.0f, glm::vec4(0.55f, 0.56f, 0.60f, 0.40f * crackAlpha));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 14.0f, 1.0f }, { 14.0f, 1.5f }, 45.0f, glm::vec4(0.50f, 0.52f, 0.56f, 0.35f * crackAlpha));
}

const Texture2D* resolveParticleTexture() {
    if (ResourceManager::hasTexture("particleTexture")) {
        return ResourceManager::getTexture("particleTexture");
    }
    return ResourceManager::loadTexture("particleTexture", "res/textures/particle.png");
}

void drawFog(SpriteRenderer* renderer, const Texture2D* whiteTexture, const Texture2D* particleTexture, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);
    float op = std::clamp(d.opacity, 0.05f, 1.0f);
    float finalAlpha = op * a;

    const Texture2D* partTex = whiteTexture;
    if (particleTexture) {
        partTex = particleTexture;
    }

    // Мягкий объемный туман / облако дыма: несколько смещенных перекрывающихся слоев
    glm::vec4 fogColor(0.48f, 0.50f, 0.52f, 0.40f * finalAlpha);
    glm::vec4 coreColor(0.52f, 0.54f, 0.56f, 0.55f * finalAlpha);
    glm::vec4 outerColor(0.44f, 0.46f, 0.48f, 0.25f * finalAlpha);

    // 1. Широкое рассеянное внешнее облако
    drawPart(renderer, partTex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 140.0f, 110.0f }, 0.0f, outerColor);
    // 2. Смещенные полупрозрачные лепестки для естественной аморфной формы
    drawPart(renderer, partTex, d.worldPos, d.rotation, sVec, { -22.0f, -10.0f }, { 110.0f, 85.0f }, 15.0f, fogColor);
    drawPart(renderer, partTex, d.worldPos, d.rotation, sVec, { 25.0f, 8.0f }, { 115.0f, 90.0f }, -12.0f, fogColor);
    drawPart(renderer, partTex, d.worldPos, d.rotation, sVec, { 8.0f, -18.0f }, { 95.0f, 75.0f }, 8.0f, fogColor);
    // 3. Более плотное центральное ядро
    drawPart(renderer, partTex, d.worldPos, d.rotation, sVec, { 2.0f, 2.0f }, { 80.0f, 65.0f }, 0.0f, coreColor);
}

void drawSelection(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d) {
    float sx = std::max(0.2f, d.scale * d.scaleX);
    float sy = std::max(0.2f, d.scale * d.scaleY);
    float baseDim = (d.type == "fog") ? 130.0f : ((d.type == "dead_tree") ? 74.0f : ((d.type == "broken_mercury_cart") ? 60.0f : ((d.type == "grass_field") ? 64.0f : ((d.type == "bush") ? 56.0f : ((d.type == "scrap_debris" || d.type == "puddle" || d.type == "ore_debris") ? 52.0f : ((d.type == "wooden_crate") ? 48.0f : 42.0f))))));
    float boxW = baseDim * sx;
    float boxH = baseDim * sy;
    float cornerLen = std::min(12.0f, std::min(boxW, boxH) * 0.35f);
    float cornerThick = 2.0f;
    glm::vec4 frameCol(0.35f, 0.85f, 1.0f, 0.95f);
    glm::vec4 dotCol(1.0f, 0.85f, 0.20f, 0.95f);

    float halfW = boxW * 0.5f;
    float halfH = boxH * 0.5f;
    glm::vec2 s1(1.0f, 1.0f);

    // Тонкая контурная рамка выделения
    float b = 1.0f;
    glm::vec4 frameLineCol(0.35f, 0.85f, 1.0f, 0.40f);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { 0.0f, -halfH + b * 0.5f }, { boxW, b }, 0.0f, frameLineCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { 0.0f, halfH - b * 0.5f }, { boxW, b }, 0.0f, frameLineCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { -halfW + b * 0.5f, 0.0f }, { b, boxH }, 0.0f, frameLineCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW - b * 0.5f, 0.0f }, { b, boxH }, 0.0f, frameLineCol);

    // 4 угловых маркера (яркие уголки)
    // Top-Left
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { -halfW + cornerLen * 0.5f, -halfH + cornerThick * 0.5f }, { cornerLen, cornerThick }, 0.0f, frameCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { -halfW + cornerThick * 0.5f, -halfH + cornerLen * 0.5f }, { cornerThick, cornerLen }, 0.0f, frameCol);
    // Top-Right
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW - cornerLen * 0.5f, -halfH + cornerThick * 0.5f }, { cornerLen, cornerThick }, 0.0f, frameCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW - cornerThick * 0.5f, -halfH + cornerLen * 0.5f }, { cornerThick, cornerLen }, 0.0f, frameCol);
    // Bottom-Left
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { -halfW + cornerLen * 0.5f, halfH - cornerThick * 0.5f }, { cornerLen, cornerThick }, 0.0f, frameCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { -halfW + cornerThick * 0.5f, halfH - cornerLen * 0.5f }, { cornerThick, cornerLen }, 0.0f, frameCol);
    // Bottom-Right
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW - cornerLen * 0.5f, halfH - cornerThick * 0.5f }, { cornerLen, cornerThick }, 0.0f, frameCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW - cornerThick * 0.5f, halfH - cornerLen * 0.5f }, { cornerThick, cornerLen }, 0.0f, frameCol);

    // Точка в центре (якорь)
    drawPart(renderer, tex, d.worldPos, 0.0f, s1, { 0.0f, 0.0f }, { 5.0f, 5.0f }, 0.0f, dotCol);

    // 5. ИНТЕРАКТИВНЫЕ РУЧКИ ТРАНСФОРМАЦИИ (GIZMO HANDLES)
    // 5.1. Правая стрелочка (X-Handle: масштаб ширины scaleX)
    glm::vec4 xLineCol(1.0f, 0.35f, 0.35f, 0.85f);
    glm::vec4 xHeadCol(1.0f, 0.32f, 0.32f, 1.0f);
    glm::vec4 xHeadCore(1.0f, 0.85f, 0.85f, 1.0f);
    glm::vec4 gizmoBgCol(0.10f, 0.12f, 0.18f, 0.95f);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW + 6.0f, 0.0f }, { 12.0f, 2.0f }, 0.0f, xLineCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW + 12.0f, 0.0f }, { 11.0f, 11.0f }, 45.0f, gizmoBgCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW + 12.0f, 0.0f }, { 8.0f, 8.0f }, 45.0f, xHeadCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW + 12.0f, 0.0f }, { 3.0f, 3.0f }, 45.0f, xHeadCore);

    // 5.2. Верхняя стрелочка (Y-Handle: масштаб высоты scaleY)
    glm::vec4 yLineCol(0.35f, 0.95f, 0.45f, 0.85f);
    glm::vec4 yHeadCol(0.30f, 0.90f, 0.40f, 1.0f);
    glm::vec4 yHeadCore(0.85f, 1.0f, 0.85f, 1.0f);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { 0.0f, -halfH - 6.0f }, { 2.0f, 12.0f }, 0.0f, yLineCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { 0.0f, -halfH - 12.0f }, { 11.0f, 11.0f }, 45.0f, gizmoBgCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { 0.0f, -halfH - 12.0f }, { 8.0f, 8.0f }, 45.0f, yHeadCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { 0.0f, -halfH - 12.0f }, { 3.0f, 3.0f }, 45.0f, yHeadCore);

    // 5.3. Угловой кружок / шарик в правом нижнем углу (Corner Scale Handle: пропорциональный масштаб)
    glm::vec4 cornerBallCol(1.0f, 0.85f, 0.20f, 1.0f);
    glm::vec4 cornerBallCore(1.0f, 1.0f, 1.0f, 1.0f);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW, halfH }, { 13.0f, 13.0f }, 45.0f, gizmoBgCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW, halfH }, { 9.0f, 9.0f }, 45.0f, cornerBallCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW - 1.0f, halfH - 1.0f }, { 3.0f, 3.0f }, 0.0f, cornerBallCore);
}

} // namespace

GizmoInfo DecorationRenderer::getGizmoInfo(const DecorationConfig& d, float cellSize) {
    const float kTileRef = 64.0f;
    float cellScaleFactor = (cellSize > 0.001f) ? (cellSize / kTileRef) : 1.0f;
    DecorationConfig dec = d;
    dec.scale *= cellScaleFactor;

    float sx = std::max(0.2f, dec.scale * dec.scaleX);
    float sy = std::max(0.2f, dec.scale * dec.scaleY);
    float baseDim = (dec.type == "fog") ? 130.0f : ((dec.type == "dead_tree") ? 74.0f : ((dec.type == "broken_mercury_cart") ? 60.0f : ((dec.type == "grass_field") ? 64.0f : ((dec.type == "bush") ? 56.0f : ((dec.type == "scrap_debris" || dec.type == "puddle" || dec.type == "ore_debris") ? 52.0f : ((dec.type == "wooden_crate") ? 48.0f : 42.0f))))));
    float boxW = baseDim * sx;
    float boxH = baseDim * sy;
    float halfW = boxW * 0.5f;
    float halfH = boxH * 0.5f;

    GizmoInfo info;
    info.boxW = boxW;
    info.boxH = boxH;
    info.cornerLocal = glm::vec2(halfW, halfH);
    info.xHandleLocal = glm::vec2(halfW + 12.0f, 0.0f);
    info.yHandleLocal = glm::vec2(0.0f, -halfH - 12.0f);

    float rad = glm::radians(dec.rotation);
    float cosR = std::cos(rad);
    float sinR = std::sin(rad);

    auto toWorld = [&](glm::vec2 local) -> glm::vec2 {
        return dec.worldPos + glm::vec2(
            local.x * cosR - local.y * sinR,
            local.x * sinR + local.y * cosR
        );
    };

    info.cornerWorld = toWorld(info.cornerLocal);
    info.xHandleWorld = toWorld(info.xHandleLocal);
    info.yHandleWorld = toWorld(info.yHandleLocal);
    return info;
}

GizmoHandleType DecorationRenderer::hitTestGizmo(
    const DecorationConfig& d,
    glm::vec2 testWorldPos,
    float hitRadius,
    float cellSize
) {
    GizmoInfo info = getGizmoInfo(d, cellSize);

    if (glm::distance(testWorldPos, info.cornerWorld) <= hitRadius) {
        return GizmoHandleType::Corner;
    }
    if (glm::distance(testWorldPos, info.xHandleWorld) <= hitRadius) {
        return GizmoHandleType::ScaleX;
    }
    if (glm::distance(testWorldPos, info.yHandleWorld) <= hitRadius) {
        return GizmoHandleType::ScaleY;
    }
    return GizmoHandleType::None;
}


void drawWoodenCrate(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Внешняя деревянная рамка и подложка
    glm::vec4 darkWood(0.24f, 0.15f, 0.08f, a);
    glm::vec4 plankCol1(0.56f, 0.36f, 0.20f, a);
    glm::vec4 plankCol2(0.48f, 0.30f, 0.17f, a);
    glm::vec4 braceCol(0.44f, 0.27f, 0.15f, a);
    glm::vec4 metalCol(0.38f, 0.40f, 0.44f, a);
    glm::vec4 rivetCol(0.85f, 0.88f, 0.92f, a);

    // Тень под ящиком
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 2.0f, 2.0f }, { 46.0f, 46.0f }, 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.35f * a));

    // Темная внешняя окантовка ящика
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 44.0f, 44.0f }, 0.0f, darkWood);

    // Внутренние продольные доски (3 горизонтальные планки)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -11.0f }, { 36.0f, 10.0f }, 0.0f, plankCol1);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f },   { 36.0f, 10.0f }, 0.0f, plankCol2);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 11.0f },  { 36.0f, 10.0f }, 0.0f, plankCol1);

    // Темные швы между досками
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -5.5f }, { 36.0f, 1.5f }, 0.0f, darkWood);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 5.5f },  { 36.0f, 1.5f }, 0.0f, darkWood);

    // Характерная диагональная деревянная планка от угла к углу
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.0f, 1.0f }, { 48.0f, 6.5f }, 45.0f, glm::vec4(0.18f, 0.11f, 0.06f, 0.65f * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 48.0f, 6.5f }, 45.0f, braceCol);

    // Внешняя рельефная деревянная рамка по периметру
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -19.5f }, { 44.0f, 5.0f }, 0.0f, darkWood);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 19.5f },  { 44.0f, 5.0f }, 0.0f, darkWood);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -19.5f, 0.0f }, { 5.0f, 44.0f }, 0.0f, darkWood);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 19.5f, 0.0f },  { 5.0f, 44.0f }, 0.0f, darkWood);

    // 4 металлических уголка с блестящими заклепками
    float cornOff = 18.0f;
    float brkSz = 7.0f;
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -cornOff, -cornOff }, { brkSz, brkSz }, 0.0f, metalCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { cornOff, -cornOff },  { brkSz, brkSz }, 0.0f, metalCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -cornOff, cornOff },  { brkSz, brkSz }, 0.0f, metalCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { cornOff, cornOff },   { brkSz, brkSz }, 0.0f, metalCol);

    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -cornOff, -cornOff }, { 2.5f, 2.5f }, 0.0f, rivetCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { cornOff, -cornOff },  { 2.5f, 2.5f }, 0.0f, rivetCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -cornOff, cornOff },  { 2.5f, 2.5f }, 0.0f, rivetCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { cornOff, cornOff },   { 2.5f, 2.5f }, 0.0f, rivetCol);
}

void drawBrokenMercuryCart(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Разлившаяся серебристо-бирюзовая лужа ртути рядом с вагонеткой
    glm::vec4 wetBorder(0.14f, 0.18f, 0.20f, 0.50f * a);
    glm::vec4 mercuryBase(0.68f, 0.88f, 0.90f, 0.88f * a);
    glm::vec4 mercuryCyan(0.24f, 0.62f, 0.68f, 0.80f * a);
    glm::vec4 mercuryHi(0.96f, 0.98f, 1.00f, 0.95f * a);

    // Темная влажная граница пятна
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 14.0f, 6.0f }, { 46.0f, 28.0f }, 8.0f, wetBorder);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 22.0f, 12.0f }, { 26.0f, 20.0f }, -12.0f, wetBorder);

    // Основная зеркальная лужа ртути
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 14.0f, 6.0f }, { 42.0f, 24.0f }, 8.0f, mercuryBase);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 20.0f, 10.0f }, { 22.0f, 16.0f }, -12.0f, mercuryBase);

    // Глубокий бирюзовый градиент глубины ртути
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 13.0f, 5.0f }, { 32.0f, 16.0f }, 8.0f, mercuryCyan);

    // Зеркальные серебристые блики на поверхности ртути
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 10.0f, 3.0f }, { 18.0f, 6.0f }, 5.0f, mercuryHi);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 22.0f, 8.0f }, { 10.0f, 4.0f }, -10.0f, mercuryHi);

    // Откатившиеся блестящие капли ртути
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 36.0f, 4.0f }, { 6.0f, 6.0f }, 0.0f, mercuryBase);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 35.0f, 3.0f }, { 2.5f, 2.5f }, 0.0f, mercuryHi);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 28.0f, -8.0f }, { 5.0f, 5.0f }, 0.0f, mercuryBase);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 27.0f, -9.0f }, { 2.0f, 2.0f }, 0.0f, mercuryHi);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 18.0f, 22.0f }, { 4.5f, 4.5f }, 0.0f, mercuryBase);

    // 2. Перевернутая ржавая шахтная вагонетка
    // Тень под корпусом
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -9.0f, -5.0f }, { 42.0f, 26.0f }, -15.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.42f * a));

    // Торчащая колесная рама и ось
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -12.0f, -14.0f }, { 34.0f, 6.0f }, -15.0f, glm::vec4(0.20f, 0.18f, 0.17f, a));
    // Колесные обода (сталь и ржавчина)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -24.0f, -17.0f }, { 8.0f, 10.0f }, -15.0f, glm::vec4(0.45f, 0.48f, 0.52f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -2.0f, -11.0f },  { 8.0f, 10.0f }, -15.0f, glm::vec4(0.45f, 0.48f, 0.52f, a));

    // Опрокинутый металлический кузов вагонетки (ржавое темное железо)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -8.0f, -4.0f }, { 38.0f, 24.0f }, -15.0f, glm::vec4(0.34f, 0.22f, 0.16f, a));

    // Ржавые разводы и коррозия на металле
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -12.0f, -6.0f }, { 18.0f, 12.0f }, -18.0f, glm::vec4(0.55f, 0.28f, 0.14f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -4.0f, 2.0f },   { 16.0f, 10.0f }, -12.0f, glm::vec4(0.48f, 0.24f, 0.12f, a));

    // Помятый стальной бортик кузова
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -7.0f, 6.0f }, { 38.0f, 4.0f }, -15.0f, glm::vec4(0.22f, 0.16f, 0.13f, a));

    // Капли ртути, стекающие по помятому борту в лужу
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 3.0f, 4.0f }, { 5.0f, 7.0f }, -15.0f, mercuryBase);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 2.5f, 3.5f }, { 2.0f, 3.0f }, -15.0f, mercuryHi);
}

void drawScrapDebris(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Кучки угля / куски черной руды
    glm::vec4 coalDark(0.11f, 0.11f, 0.13f, a);
    glm::vec4 coalMid(0.18f, 0.18f, 0.22f, a);
    glm::vec4 coalHi(0.32f, 0.34f, 0.38f, a);

    // Тень под обломками
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.0f, 1.0f }, { 44.0f, 38.0f }, 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.30f * a));

    // Угольные глыбы слева
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -14.0f, 8.0f }, { 16.0f, 13.0f }, 25.0f, coalDark);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -13.0f, 7.0f }, { 10.0f, 8.0f },  25.0f, coalMid);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -11.0f, 6.0f }, { 4.0f, 4.0f },   25.0f, coalHi);

    // Угольные глыбы справа
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 13.0f, 10.0f }, { 15.0f, 12.0f }, -20.0f, coalDark);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 12.0f, 9.0f },  { 9.0f, 7.0f },   -20.0f, coalMid);

    // Мелкие куски руды сверху
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 3.0f, -12.0f }, { 10.0f, 8.0f }, 15.0f, coalDark);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 2.0f, -13.0f }, { 5.0f, 4.0f },  15.0f, coalHi);

    // 2. Разбросанные деревянные доски и обломки ящиков
    glm::vec4 wood1(0.56f, 0.38f, 0.24f, a);
    glm::vec4 wood2(0.44f, 0.28f, 0.17f, a);
    glm::vec4 wood3(0.62f, 0.44f, 0.28f, a);
    glm::vec4 nailCol(0.80f, 0.82f, 0.86f, a);

    // Доска 1 (длинная, наискосок)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, -3.0f }, { 34.0f, 7.0f }, 26.0f, wood1);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 11.0f, 2.0f },  { 2.0f, 2.5f },  26.0f, nailCol);

    // Доска 2 (перекрестная, темная выветренная)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 5.0f, 2.0f }, { 28.0f, 6.0f }, -38.0f, wood2);

    // Доска 3 (короткий отщеп доски ящика)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -9.0f, -6.0f }, { 18.0f, 6.0f }, 72.0f, wood3);

    // Щепки и обломки ящика
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 15.0f, -7.0f }, { 10.0f, 4.5f }, -12.0f, wood2);
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -6.0f, 13.0f }, { 12.0f, 4.0f }, 40.0f,  wood1);
}


void drawDeadTree(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Падающая тень на земле
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 10.0f, 16.0f }, { 38.0f, 16.0f }, 15.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.35f * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 24.0f, 24.0f }, { 48.0f, 10.0f }, 32.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.28f * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 38.0f, 28.0f }, { 28.0f, 6.0f }, 45.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.22f * a));

    // 2. Корни и основание ствола
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -9.0f, 14.0f }, { 13.0f, 7.0f }, -25.0f, glm::vec4(0.20f, 0.16f, 0.12f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 8.0f, 15.0f }, { 15.0f, 8.0f }, 20.0f, glm::vec4(0.16f, 0.13f, 0.10f, a));

    // 3. Нижняя часть ствола
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 6.0f }, { 12.0f, 18.0f }, -4.0f, glm::vec4(0.26f, 0.21f, 0.16f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -3.5f, 6.0f }, { 3.5f, 18.0f }, -4.0f, glm::vec4(0.42f, 0.35f, 0.26f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 3.5f, 6.0f }, { 3.5f, 18.0f }, -4.0f, glm::vec4(0.14f, 0.11f, 0.08f, a));

    // 4. Средняя часть ствола (изгиб)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.5f, -9.0f }, { 9.0f, 20.0f }, 8.0f, glm::vec4(0.28f, 0.23f, 0.17f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.5f, -9.0f }, { 2.5f, 20.0f }, 8.0f, glm::vec4(0.44f, 0.37f, 0.28f, a));

    // 5. Левая крупная ветвь с сучьями
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -13.0f, -15.0f }, { 22.0f, 6.5f }, -35.0f, glm::vec4(0.30f, 0.24f, 0.18f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -22.0f, -23.0f }, { 15.0f, 4.0f }, -60.0f, glm::vec4(0.33f, 0.26f, 0.20f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -17.0f, -11.0f }, { 11.0f, 3.5f }, -15.0f, glm::vec4(0.25f, 0.20f, 0.15f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -28.0f, -28.0f }, { 9.0f, 2.5f }, -75.0f, glm::vec4(0.42f, 0.35f, 0.27f, a));

    // 6. Правая крупная ветвь с разветвлениями
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 15.0f, -17.0f }, { 24.0f, 7.0f }, 38.0f, glm::vec4(0.27f, 0.22f, 0.16f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 24.0f, -25.0f }, { 17.0f, 4.0f }, 65.0f, glm::vec4(0.30f, 0.24f, 0.18f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 26.0f, -13.0f }, { 13.0f, 3.5f }, 15.0f, glm::vec4(0.22f, 0.17f, 0.13f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 30.0f, -32.0f }, { 10.0f, 2.5f }, 80.0f, glm::vec4(0.38f, 0.31f, 0.24f, a));

    // 7. Центральная макушка кроны (сухие острые пики)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 3.0f, -26.0f }, { 6.5f, 18.0f }, -6.0f, glm::vec4(0.32f, 0.26f, 0.19f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -3.0f, -36.0f }, { 11.0f, 3.0f }, -30.0f, glm::vec4(0.36f, 0.29f, 0.22f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 9.0f, -37.0f }, { 12.0f, 3.0f }, 25.0f, glm::vec4(0.40f, 0.33f, 0.25f, a));

    // Сучок на стволе
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.0f, 2.0f }, { 3.5f, 6.0f }, 0.0f, glm::vec4(0.12f, 0.09f, 0.07f, a));
}

void drawDryGrass(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Тень под кочкой бурьяна
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 3.0f, 13.0f }, { 32.0f, 10.0f }, 0.0f, glm::vec4(0.06f, 0.05f, 0.04f, 0.32f * a));

    // 2. Густое плотное основание дерновины
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 8.0f }, { 22.0f, 9.0f }, 0.0f, glm::vec4(0.24f, 0.18f, 0.10f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 6.0f }, { 17.0f, 7.0f }, 0.0f, glm::vec4(0.32f, 0.24f, 0.12f, a));

    // 3. Задние темные стебли сухой травы
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -11.0f, 0.0f }, { 4.0f, 19.0f }, -35.0f, glm::vec4(0.42f, 0.34f, 0.16f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 11.0f, 1.0f }, { 4.0f, 18.0f }, 32.0f, glm::vec4(0.40f, 0.32f, 0.15f, a));

    // 4. Основной веер желто-бурых стеблей
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -7.0f, -4.0f }, { 3.5f, 25.0f }, -18.0f, glm::vec4(0.56f, 0.46f, 0.22f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -8.0f }, { 4.0f, 29.0f }, 2.0f, glm::vec4(0.62f, 0.52f, 0.25f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 6.0f, -6.0f }, { 3.5f, 26.0f }, 15.0f, glm::vec4(0.58f, 0.48f, 0.23f, a));

    // 5. Выжженные солнцем сухие метелки / колоски
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -15.0f, -7.0f }, { 3.0f, 17.0f }, -52.0f, glm::vec4(0.68f, 0.58f, 0.28f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 14.0f, -8.0f }, { 3.0f, 19.0f }, 48.0f, glm::vec4(0.66f, 0.56f, 0.27f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -2.0f, -19.0f }, { 3.0f, 13.0f }, -8.0f, glm::vec4(0.74f, 0.65f, 0.32f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 4.0f, -20.0f }, { 3.0f, 14.0f }, 10.0f, glm::vec4(0.72f, 0.62f, 0.30f, a));

    // 6. Поникшие надломленные сухие былинки
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 17.0f, 5.0f }, { 13.0f, 2.5f }, 68.0f, glm::vec4(0.48f, 0.38f, 0.18f, a));
}

void drawOreDebris(SpriteRenderer* renderer, const Texture2D* tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Падающая тень под осыпью
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 2.0f, 6.0f }, { 44.0f, 23.0f }, 0.0f, glm::vec4(0.05f, 0.05f, 0.07f, 0.45f * a));

    // 2. Центральный крупный валун породы (сланец / доломит)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.0f, 3.0f }, { 26.0f, 18.0f }, 4.0f, glm::vec4(0.16f, 0.17f, 0.19f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 0.0f }, { 24.0f, 16.0f }, -3.0f, glm::vec4(0.32f, 0.33f, 0.36f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -4.0f, -3.0f }, { 16.0f, 10.0f }, -8.0f, glm::vec4(0.48f, 0.50f, 0.54f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -5.0f, -5.0f }, { 10.0f, 4.0f }, -12.0f, glm::vec4(0.65f, 0.68f, 0.72f, a));

    // 3. Массивные вкрапления киновари (HgS) на центральном камне
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -1.0f }, { 12.0f, 7.0f }, 15.0f, glm::vec4(0.55f, 0.07f, 0.10f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, -2.0f }, { 8.0f, 4.0f }, 18.0f, glm::vec4(0.82f, 0.12f, 0.16f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -2.0f, -3.0f }, { 4.0f, 2.0f }, 20.0f, glm::vec4(0.96f, 0.28f, 0.25f, a));

    // 4. Левый камень осыпи
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -14.0f, 4.0f }, { 16.0f, 12.0f }, -14.0f, glm::vec4(0.24f, 0.25f, 0.28f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -16.0f, 2.0f }, { 11.0f, 7.0f }, -18.0f, glm::vec4(0.44f, 0.46f, 0.50f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -13.0f, 3.0f }, { 5.0f, 4.0f }, 5.0f, glm::vec4(0.70f, 0.09f, 0.12f, a));

    // 5. Правый камень осыпи
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 14.0f, 3.0f }, { 17.0f, 13.0f }, 22.0f, glm::vec4(0.26f, 0.27f, 0.30f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 13.0f, 1.0f }, { 12.0f, 6.0f }, 20.0f, glm::vec4(0.46f, 0.48f, 0.52f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 15.0f, 2.0f }, { 6.0f, 3.5f }, -10.0f, glm::vec4(0.75f, 0.10f, 0.14f, a));

    // 6. Мелкие осколки и киноварная крошка
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -19.0f, 9.0f }, { 5.0f, 4.0f }, 8.0f, glm::vec4(0.36f, 0.38f, 0.40f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 6.0f, 9.0f }, { 6.0f, 4.5f }, -5.0f, glm::vec4(0.68f, 0.12f, 0.15f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 21.0f, 8.0f }, { 5.5f, 4.0f }, 30.0f, glm::vec4(0.30f, 0.32f, 0.34f, a));
}

static void renderDecorationImpl(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const Texture2D* particleTexture,
    const DecorationConfig& decor,
    bool isSelected,
    float alpha,
    float cellSize
) {
    if (!renderer || !whiteTexture) return;

    const float kTileRef = 64.0f;
    float cellScaleFactor = 1.0f;
    if (cellSize > 0.001f) {
        cellScaleFactor = cellSize / kTileRef;
    }

    DecorationConfig d = decor;
    d.scale *= cellScaleFactor;

    if (d.type == "bush") {
        drawBush(renderer, whiteTexture, d, alpha);
    } else if (d.type == "grass_tuft") {
        drawGrassTuft(renderer, whiteTexture, d, alpha);
    } else if (d.type == "grass_field") {
        drawGrassField(renderer, whiteTexture, d, alpha);
    } else if (d.type == "flower") {
        drawFlower(renderer, whiteTexture, d, alpha);
    } else if (d.type == "stone") {
        drawStone(renderer, whiteTexture, d, alpha);
    } else if (d.type == "helmet") {
        drawHelmet(renderer, whiteTexture, d, alpha);
    } else if (d.type == "pickaxe") {
        drawPickaxe(renderer, whiteTexture, d, alpha);
    } else if (d.type == "puddle") {
        drawPuddle(renderer, whiteTexture, d, alpha);
    } else if (d.type == "crack") {
        drawCrack(renderer, whiteTexture, d, alpha);
    } else if (d.type == "fog") {
        const Texture2D* fogTex = particleTexture;
        if (!fogTex) {
            fogTex = resolveParticleTexture();
        }
        drawFog(renderer, whiteTexture, fogTex, d, alpha);
    } else if (d.type == "wooden_crate") {
        drawWoodenCrate(renderer, whiteTexture, d, alpha);
    } else if (d.type == "broken_mercury_cart") {
        drawBrokenMercuryCart(renderer, whiteTexture, d, alpha);
    } else if (d.type == "scrap_debris") {
        drawScrapDebris(renderer, whiteTexture, d, alpha);
    } else if (d.type == "dead_tree") {
        drawDeadTree(renderer, whiteTexture, d, alpha);
    } else if (d.type == "dry_grass") {
        drawDryGrass(renderer, whiteTexture, d, alpha);
    } else if (d.type == "ore_debris") {
        drawOreDebris(renderer, whiteTexture, d, alpha);
    } else {
        // Запасной вариант для неизвестного типа
        drawBush(renderer, whiteTexture, d, alpha);
    }

    if (isSelected) {
        drawSelection(renderer, whiteTexture, d);
    }
}

void DecorationRenderer::renderDecoration(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const DecorationConfig& decor,
    bool isSelected,
    float alpha,
    float cellSize
) {
    renderDecorationImpl(renderer, whiteTexture, nullptr, decor, isSelected, alpha, cellSize);
}

void DecorationRenderer::renderDecoration(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    glm::vec2 renderPos,
    const DecorationConfig& decor,
    bool isSelected,
    float alpha,
    float cellSize
) {
    DecorationConfig decCopy = decor;
    decCopy.worldPos = renderPos;
    renderDecorationImpl(renderer, whiteTexture, nullptr, decCopy, isSelected, alpha, cellSize);
}

int DecorationRenderer::sortByZIndex(std::vector<DecorationConfig>& decorations, int selectedIndex) {
    if (decorations.size() <= 1) {
        return selectedIndex;
    }

    // Быстрое стабильное переупорядочивание одного изменённого элемента без аллокаций
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(decorations.size())) {
        int idx = selectedIndex;
        while (idx > 0 && decorations[idx].zIndex < decorations[idx - 1].zIndex) {
            std::swap(decorations[idx], decorations[idx - 1]);
            --idx;
        }
        while (idx + 1 < static_cast<int>(decorations.size()) && decorations[idx].zIndex > decorations[idx + 1].zIndex) {
            std::swap(decorations[idx], decorations[idx + 1]);
            ++idx;
        }
        return idx;
    }

    std::stable_sort(decorations.begin(), decorations.end(), [](const DecorationConfig& a, const DecorationConfig& b) {
        return a.zIndex < b.zIndex;
    });
    return selectedIndex;
}

void DecorationRenderer::renderDecorations(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const std::vector<DecorationConfig>& decorations,
    int selectedIndex,
    float cellSize,
    bool onlyFog
) {
    if (!renderer || !whiteTexture || decorations.empty()) return;

    const Texture2D* particleTexture = nullptr;
    if (onlyFog) {
        particleTexture = resolveParticleTexture();
    }

    for (size_t i = 0; i < decorations.size(); ++i) {
        bool isFog = (decorations[i].type == "fog" || decorations[i].type == "darkness");
        if (onlyFog != isFog) continue;
        bool isSel = (static_cast<int>(i) == selectedIndex);
        renderDecorationImpl(renderer, whiteTexture, particleTexture, decorations[i], isSel, 1.0f, cellSize);
    }
}

void DecorationRenderer::renderDecorations(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const std::vector<DecorationConfig>& decorations,
    glm::vec2 gridOffset,
    float cellSize,
    int selectedIndex,
    bool onlyFog
) {
    if (!renderer || !whiteTexture || decorations.empty()) return;

    const Texture2D* particleTexture = nullptr;
    if (onlyFog) {
        particleTexture = resolveParticleTexture();
    }

    for (size_t i = 0; i < decorations.size(); ++i) {
        bool isFog = (decorations[i].type == "fog" || decorations[i].type == "darkness");
        if (onlyFog != isFog) continue;
        bool isSel = (static_cast<int>(i) == selectedIndex);
        DecorationConfig decCopy = decorations[i];
        decCopy.worldPos = gridOffset + decorations[i].tilePos * cellSize;
        renderDecorationImpl(renderer, whiteTexture, particleTexture, decCopy, isSel, 1.0f, cellSize);
    }
}

void DecorationRenderer::renderDecorations(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const std::vector<DecorationConfig>& decorations,
    const Grid& grid,
    int selectedIndex,
    bool onlyFog
) {
    renderDecorations(renderer, whiteTexture, decorations, grid.getOffset(), grid.getCellSize(), selectedIndex, onlyFog);
}

void DecorationRenderer::renderPreview(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const std::string& type,
    glm::vec2 worldPos,
    float rotation,
    float scale,
    float scaleX,
    float scaleY,
    float opacity,
    float cellSize
) {
    DecorationConfig d;
    d.type = type;
    d.worldPos = worldPos;
    d.rotation = rotation;
    d.scale = scale;
    d.scaleX = scaleX;
    d.scaleY = scaleY;
    d.opacity = opacity;
    renderDecoration(renderer, whiteTexture, d, false, 0.65f, cellSize);
}

