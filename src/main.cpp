#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>
#include <memory>
#include "resources/ResourceManager.h"
#include "game/core/Game.h"
#include "game/core/InputManager.h"
#include "audio/AudioManager.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

int windowWidth = 1280; // логическая ширина окна (совпадает с координатами glfwGetCursorPos)
int windowHeight = 720; // логическая высота окна

std::unique_ptr<Game> TowerDefenseGame; // указатель на игру, который будет использоваться в коллбеках и главном цикле


/*
Эту функцию вызывает GLFW при изменении физического размера буфера кадра (с учётом HiDPI / devicePixelRatio)
*/
void glfwFramebufferSizeCallback(GLFWwindow* window, int fbWidth, int fbHeight)
{
    (void)window;
    if (fbWidth <= 0 || fbHeight <= 0) return;
    glViewport(0, 0, fbWidth, fbHeight);
}

/*
Эту функцию вызывает сама система через GLFW, когда изменяется логический размер окна.
Обновляет глобальные переменные размера окна (windowWidth, windowHeight)
и пересчитывает проекцию/раскладку игры под логические координаты мыши.
*/
void glfwWindowSizeCallback(GLFWwindow* window, int width, int height)
{
    // защита от свертывания
    if (width <= 0 || height <= 0) return;

    // обновляем глобальные переменные логического размера окна
    windowWidth = width;
    windowHeight = height;

    // область рисования OpenGL выставляем по реальному размеру framebuffer (HiDPI-safe)
    int fbWidth = width;
    int fbHeight = height;
    if (window) {
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    }
    if (fbWidth > 0 && fbHeight > 0) {
        glViewport(0, 0, fbWidth, fbHeight);
    }

    // если игра создана
    if (TowerDefenseGame) {
        // сообщаем игре о новом логическом размере окна
        // чтобы матрица проекции и UI совпадали 1:1 с glfwGetCursorPos
        TowerDefenseGame->resize(width, height);
    }
}


// Эту функцию вызывает сама система через GLFW, когда пользователь нажимает клавишу
void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    (void)window; (void)key; (void)scancode; (void)action; (void)mode;
}

#ifdef __EMSCRIPTEN__
static GLFWwindow* g_webWindow = nullptr;
static double g_lastFrameTime = 0.0;
static float g_timeAccumulator = 0.0f;
static const float FIXED_DT = 1.0f / 60.0f;

static void computeWebLogicalSize(int& outW, int& outH) {
    double cssW = 0.0;
    double cssH = 0.0;
    // Измеряем именно #game-container (а не #canvas), т.к. GLFW_SCALE_TO_MONITOR
    // выставляет inline style width/height на #canvas.
    if (emscripten_get_element_css_size("#game-container", &cssW, &cssH) != EMSCRIPTEN_RESULT_SUCCESS ||
        cssW <= 0.0 || cssH <= 0.0) {
        outW = windowWidth;
        outH = windowHeight;
        return;
    }

    int logicalW = static_cast<int>(std::round(cssW));
    int logicalH = static_cast<int>(std::round(cssH));
    if (logicalW < 320) logicalW = 320;
    if (logicalH < 180) logicalH = 180;

    outW = logicalW;
    outH = logicalH;
}

static void syncWebCanvasSize() {
    if (!g_webWindow) return;

    int logicalW = windowWidth;
    int logicalH = windowHeight;
    computeWebLogicalSize(logicalW, logicalH);

    if (logicalW != windowWidth || logicalH != windowHeight) {
        // При GLFW_SCALE_TO_MONITOR = GLFW_TRUE Emscripten GLFW сам умножит
        // canvas.width/height на devicePixelRatio для чёткого рендера,
        // а windowWidth/windowHeight и glfwGetCursorPos останутся в CSS-пикселях (1:1).
        glfwSetWindowSize(g_webWindow, logicalW, logicalH);
        if (windowWidth != logicalW || windowHeight != logicalH) {
            glfwWindowSizeCallback(g_webWindow, logicalW, logicalH);
        }
    } else {
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(g_webWindow, &fbW, &fbH);
        if (fbW > 0 && fbH > 0) {
            glViewport(0, 0, fbW, fbH);
        }
    }
}

static EM_BOOL emscriptenResizeCallback(int /*eventType*/, const EmscriptenUiEvent* /*uiEvent*/, void* /*userData*/) {
    syncWebCanvasSize();
    return EM_FALSE;
}

static EM_BOOL emscriptenFullscreenCallback(int /*eventType*/, const EmscriptenFullscreenChangeEvent* /*fullscreenChangeEvent*/, void* /*userData*/) {
    syncWebCanvasSize();
    return EM_FALSE;
}

static void emscriptenMainLoopStep() {
    if (g_webWindow && glfwWindowShouldClose(g_webWindow)) {
        std::cout << "[Web] Warning: Window close request ignored to keep application running in browser." << std::endl;
        glfwSetWindowShouldClose(g_webWindow, GLFW_FALSE);
    }

    double currentFrame = glfwGetTime();
    float frameTime = static_cast<float>(currentFrame - g_lastFrameTime);
    g_lastFrameTime = currentFrame;

    if (frameTime > 0.25f) {
        frameTime = 0.25f;
    }

    g_timeAccumulator += frameTime;

    glfwPollEvents();

    if (TowerDefenseGame) {
        TowerDefenseGame->processInput(g_webWindow, frameTime);

        while (g_timeAccumulator >= FIXED_DT) {
            TowerDefenseGame->update(FIXED_DT);
            g_timeAccumulator -= FIXED_DT;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        TowerDefenseGame->render();
    }

    glfwSwapBuffers(g_webWindow);
    InputManager::clearFrameText();
}
#endif


int main(void)
{
    // Запускаем GLFW и настраиваем контекст OpenGL
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

#ifdef __EMSCRIPTEN__
    // WebGL 2.0 соответствует OpenGL ES 3.0
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    // Включаем нативную поддержку HiDPI в Emscripten GLFW:
    // логический размер окна и glfwGetCursorPos в CSS-пикселях,
    // а WebGL framebuffer автоматически масштабируется на devicePixelRatio.
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    // Сразу берем актуальный адаптивный CSS-размер контейнера
    computeWebLogicalSize(windowWidth, windowHeight);
#else
    // Просим GLFW создать контекст OpenGL 4.6 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    /* Создаем окно программы с названием которое можно задать */
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "IASA tower defence", nullptr, nullptr);

    // Если окно не удалось создать, то выводим ошибку и завершаем работу
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Привязываем функции колбеки к созданному окну
    glfwSetWindowSizeCallback(window, glfwWindowSizeCallback);
    glfwSetFramebufferSizeCallback(window, glfwFramebufferSizeCallback);
    glfwSetKeyCallback(window, glfwKeyCallback);
    InputManager::init(window);

    // Делаем контекст этого окна главным для текущего потока процессора
    glfwMakeContextCurrent(window);

#ifndef __EMSCRIPTEN__
    // Библиотека GLAD ищет на ПК драйвер видеокарты и подвязывает к OpenGL
    if (!gladLoadGL()) {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << std::endl;
#else
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL ES version: " << glGetString(GL_VERSION) << std::endl;
#endif

    int initFbW = windowWidth;
    int initFbH = windowHeight;
    glfwGetFramebufferSize(window, &initFbW, &initFbH);
    glViewport(0, 0, initFbW > 0 ? initFbW : windowWidth, initFbH > 0 ? initFbH : windowHeight);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f); // Задаем цвет фона
    glEnable(GL_BLEND); // Включение прозрачности
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Включение прозрачности

    // инициализация аудио
    AudioManager::init();

    // создание и инициализация игры
    TowerDefenseGame = std::make_unique<Game>(windowWidth, windowHeight);
    TowerDefenseGame->init();

#ifdef __EMSCRIPTEN__
    g_webWindow = window;
    syncWebCanvasSize();
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_FALSE, emscriptenResizeCallback);
    emscripten_set_fullscreenchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_FALSE, emscriptenFullscreenCallback);

    g_lastFrameTime = glfwGetTime();
    g_timeAccumulator = 0.0f;
    emscripten_set_main_loop(emscriptenMainLoopStep, 0, 1);
#else
    // СУПЕР ЧЕТЕНЬКАЯ НАЙСТРОЙКА ФИКСИРОВАНЫХ ТИКОВ
    const float FIXED_DT = 1.0f / 60.0f; // 60 тиков в секунду

    double lastFrame = glfwGetTime();
    float accumulator = 0.0f;

    // Главный цикл игры, который будет работать пока окно не закроется
    while (!glfwWindowShouldClose(window))
    {
        double currentFrame = glfwGetTime();
        float frameTime = static_cast<float>(currentFrame - lastFrame);
        lastFrame = currentFrame;

        if (frameTime > 0.25f) {
            frameTime = 0.25f;
        }

        accumulator += frameTime;

        glfwPollEvents();

        TowerDefenseGame->processInput(window, frameTime);

        while (accumulator >= FIXED_DT) {
            TowerDefenseGame->update(FIXED_DT);
            accumulator -= FIXED_DT;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        TowerDefenseGame->render();
        glfwSwapBuffers(window);

        InputManager::clearFrameText();
    }

    AudioManager::cleanup();
    TowerDefenseGame.reset();
    ResourceManager::clear();
    glfwTerminate();
    return 0;
#endif
}