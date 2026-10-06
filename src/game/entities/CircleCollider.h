#pragma once
#include <glm/glm.hpp>

struct CircleCollider {
    glm::vec2 center; // координаті центра
    float radius; // радиус окружности

    // функция проверки столкновение с другой окружностью
    bool intersects(const CircleCollider& other) const {
        float x = center.x - other.center.x;
        float y = center.y - other.center.y;


        float distanceSq = x * x + y * y;// считаем квадрат растояния
        float radiiSum = radius + other.radius; // считаем квадрат суммі радиусов
        return distanceSq <= (radiiSum * radiiSum);
    }
};