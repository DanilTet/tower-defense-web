#include "Game.h"
#include "resources/ResourceManager.h"
#include "SettingsManager.h"
#include "LocalizationManager.h"
#include "../states/GameplayState.h" 
#include "../states/MainMenuState.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Конструктор и деструктор
Game::Game(int width, int height) : width(width), height(height) {}

Game::~Game() {}

// Инициализация игры, загрузка ресурсов, настройка рендерера и создание сетки
void Game::init() {
    // ЗАГРУЗКА БАЗОВЫХ ШЕЙДЕРОВ
    if (!ResourceManager::loadShader("spriteShader", "res/shaders/vertex_shader.vert", "res/shaders/fragment_shader.frag")) {
        std::cerr << "Failed to load shaders" << std::endl;
    }

    // ШЕЙДЕР
	// Получаем указатель на шейдер из ResourceManager и создаем рендерер для спрайтов
    ShaderProgram* shader = ResourceManager::getShader("spriteShader");

	// Обертываем его в пустой shared_ptr, чтобы SpriteRenderer не удалял его при уничтожении, так как ResourceManager управляет временем жизни шейдера
    std::shared_ptr<ShaderProgram> shaderPtr(shader, [](ShaderProgram*) {});
    
    // Создаем обьект рендера и передаем туда шейдер
    m_renderer = std::make_unique<SpriteRenderer>(shaderPtr);

	// Создаем  ортографическую матрицу лево = 0, право = ширина окна, низ = высота окна, верх = 0, ближняя плоскость = -1, дальняя плоскость = 1
    // после єтого все координаты в игре будут в пикселях, где (0, 0) - это верхний левый угол окна, а (width, height) - это нижний правый угол окна
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->width), static_cast<float>(this->height), 0.0f, -1.0f, 1.0f);
    
	// Загружаем єту матрицу в рендерер, чтобы он использовал ее при отрисовке спрайтов
    m_renderer->setProjection(projection);

    // Включаем шейдер в работу на GPU
    shader->use();

    // Привязываем uniform-переменную текстуры в фрагментном шейдере к текстурному слоту №0
    glUniform1i(glGetUniformLocation(shader->getId(), "u_texture"), 0);

    // ИНИЦИАЛИЗАЦИЯ ТЕКСТА
    if (!ResourceManager::loadShader("textShader", "res/shaders/text_shader.vert", "res/shaders/text_shader.frag")) {
        std::cerr << "Failed to load text shaders" << std::endl;
    }
    ShaderProgram* textShader = ResourceManager::getShader("textShader");
    std::shared_ptr<ShaderProgram> textShaderPtr(textShader, [](ShaderProgram*) {});
    
    // Создаем рендерер текста
    m_textRenderer = std::make_unique<TextRenderer>(textShaderPtr, this->width, this->height);
    if (!m_textRenderer->Load("res/fonts/Roboto-Regular.ttf", 24)) {
        std::cerr << "Failed to load font!" << std::endl;
    }
    SettingsManager::init();
    LocalizationManager::init();

    // Загружаем шрифт с размером 24 пикселя
    m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, width, height, m_renderer, m_textRenderer.get()));
}

// Обработка ввода вызывается каждый кадр
void Game::processInput(GLFWwindow* window, float dt) {
    m_stateManager.processInput(window, dt);
}

// Обновление игровой логики вызывается каждый кадр, после обработки ввода
void Game::update(float dt) {
    m_stateManager.update(dt);
    m_stateManager.applyPendingChanges();
}

// Отрисовка кадра вызывается каждый кадр, после обновления логики
void Game::render() {
    m_stateManager.render();
}

// Изменение размеров окна: вызывается системным колбеком из main.cpp
void Game::resize(int width, int height) {
    this->width = width; // Перезаписываем новое значение ширины игры
    this->height = height; // Перезаписываем новое значение высоты игры

    if (m_renderer) { // Если рендерер уже создан
        // Заново создаем ортографическую матрицу под новые физические размеры окна
        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
        m_renderer->setProjection(projection); // Загружаем обновленную матрицу в рендерер
    }

    if (m_textRenderer) {
        glm::mat4 textProjection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f);
        m_textRenderer->updateProjection(textProjection);
    }

    m_stateManager.resize(width, height);
}
