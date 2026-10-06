#include "SpriteRenderer.h"
#include "shaders/ShaderProgram.h"
#include "textures/Texture2D.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

SpriteRenderer::SpriteRenderer(std::shared_ptr<ShaderProgram> shader)
    : m_shader(std::move(shader))
{
    this->initRenderData();
    m_vertices.resize(MAX_VERTICES);
}

SpriteRenderer::~SpriteRenderer() {
    glDeleteVertexArrays(1, &m_quadVAO);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
}

SpriteRenderer::SpriteRenderer(SpriteRenderer&& other) noexcept
    : m_shader(std::move(other.m_shader)),
    m_quadVAO(other.m_quadVAO),
    m_vbo(other.m_vbo),
    m_ebo(other.m_ebo)
{
    other.m_quadVAO = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
}

SpriteRenderer& SpriteRenderer::operator=(SpriteRenderer&& other) noexcept {
    if (this != &other) {
        glDeleteVertexArrays(1, &m_quadVAO);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);

        m_shader = std::move(other.m_shader);
        m_quadVAO = other.m_quadVAO;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;

        other.m_quadVAO = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
    }
    return *this;
}

void SpriteRenderer::initRenderData() {
    float vertices[] = {
        0.0f, 0.0f,    0.0f, 0.0f, // Левый верхний угол
        1.0f, 0.0f,    1.0f, 0.0f, // Правый верхний угол
        1.0f, 1.0f,    1.0f, 1.0f, // Правый нижний угол
        0.0f, 1.0f,    0.0f, 1.0f  // Левый нижний угол
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(SpriteVertex), nullptr, GL_DYNAMIC_DRAW);

    // генерируем индексы заранее
    std::vector<unsigned int> generatedIndices(MAX_INDICES);
    int offset = 0;
    for (int i = 0; i < MAX_INDICES; i += 6) {
        generatedIndices[i + 0] = offset + 0;
        generatedIndices[i + 1] = offset + 1;
        generatedIndices[i + 2] = offset + 2;
        generatedIndices[i + 3] = offset + 2;
        generatedIndices[i + 4] = offset + 3;
        indicesData: indicesText: generatedIndices[i + 5] = offset + 0;
        offset += 4;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, generatedIndices.size() * sizeof(unsigned int), generatedIndices.data(), GL_STATIC_DRAW);

    // атрибут 0:vec2 позиция
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, position));

    // атрибут 1: vec2 текстурные координаты
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, texCoords));

    // атрибут 2: vec4 цвет
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, color));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SpriteRenderer::setProjection(const glm::mat4& projection) {
    m_shader->use();
    glUniformMatrix4fv(glGetUniformLocation(m_shader->getId(), "u_projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void SpriteRenderer::beginBatch() {
    m_spriteCount = 0;
    m_currentTexture = nullptr;
}

void SpriteRenderer::endBatch() {
    flush(); // в конце кадра рисуем все что осталось в буфере
}

void SpriteRenderer::flush() {
    if (m_spriteCount == 0) return; // нету чего рисовать

    // привязываем нужную текстуру
    if (m_currentTexture) {
        m_currentTexture->bind(0);
    }

    m_shader->use();
    glBindVertexArray(m_quadVAO);

    // загружаем наш массив вершин в память видеокарты
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_spriteCount * 4 * sizeof(SpriteVertex), m_vertices.data());

    // ОДИН ВЫЗОВ ОТРИСОВКИ ДЛЯ ВСЕХ СПРАЙТОВ ЭТОЙ ТЕКСТУРЫ!
    glDrawElements(GL_TRIANGLES, m_spriteCount * 6, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);

    // сбрасываем счетчик
    m_spriteCount = 0;
}

void SpriteRenderer::drawSprite(const std::shared_ptr<Texture2D>& texture, glm::vec2 position, glm::vec2 size, float rotation, glm::vec3 color, SpriteUV uv) {
    // перенаправляем на RGBA
    drawSpriteRGBA(texture, position, size, rotation, glm::vec4(color, 1.0f), uv);
}

void SpriteRenderer::drawSpriteRGBA(const std::shared_ptr<Texture2D>& texture, glm::vec2 position, glm::vec2 size, float rotation, glm::vec4 color, SpriteUV uv) {
    // рисуем то, что накопили
    if (m_currentTexture != texture || m_spriteCount >= MAX_SPRITES) {
        flush();
        m_currentTexture = texture;
    }

    // вручную считаем трансформацию матрицы
    glm::vec2 center = position + size * 0.5f;

    float cosA = cos(glm::radians(rotation));
    float sinA = sin(glm::radians(rotation));

    float halfW = size.x * 0.5f;
    float halfH = size.y * 0.5f;

    // углы относительно центра
    glm::vec2 tl(-halfW, -halfH);
    glm::vec2 tr(halfW, -halfH);
    glm::vec2 br(halfW, halfH);
    glm::vec2 bl(-halfW, halfH);

    // функция поворота и смещения егор
    auto transform = [&](glm::vec2 p) {
        return glm::vec2(
            center.x + (p.x * cosA - p.y * sinA),
            center.y + (p.x * sinA + p.y * cosA)
        );
    };

    // записываем 4 вершины в массив оперативной памяти
    int index = m_spriteCount * 4;

    m_vertices[index + 0].position = transform(tl);
    m_vertices[index + 0].texCoords = glm::vec2(uv.uvMin.x, uv.uvMin.y);
    m_vertices[index + 0].color = color;

    m_vertices[index + 1].position = transform(tr);
    m_vertices[index + 1].texCoords = glm::vec2(uv.uvMax.x, uv.uvMin.y);
    m_vertices[index + 1].color = color;

    m_vertices[index + 2].position = transform(br);
    m_vertices[index + 2].texCoords = glm::vec2(uv.uvMax.x, uv.uvMax.y);
    m_vertices[index + 2].color = color;

    m_vertices[index + 3].position = transform(bl);
    m_vertices[index + 3].texCoords = glm::vec2(uv.uvMin.x, uv.uvMax.y);
    m_vertices[index + 3].color = color;

    m_spriteCount++;
}