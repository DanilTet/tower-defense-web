#pragma once
#include <memory>
#include <vector>
#include <string>
#include "IGameState.h"

class SpriteRenderer;
class TextRenderer;

class GameStateManager {
private:
    // типа стек состояний самый верхнее состояние оно активное
    std::vector<std::unique_ptr<IGameState>> m_states;

    // флаги для безопасного переключения в конце кадра
    std::unique_ptr<IGameState> m_nextState;
    bool m_clearAllAndSet = false;
    int m_popCount = 0;

    bool m_returnToEditorRequested = false;
    std::string m_editorFallbackLevel = "";
    int m_editorWidth = 1280;
    int m_editorHeight = 720;
    std::shared_ptr<SpriteRenderer> m_editorRenderer;
    TextRenderer* m_editorTextRenderer = nullptr;

public:
    GameStateManager() = default;
    ~GameStateManager();

    // полная смена состояния
    void setState(std::unique_ptr<IGameState> newState);

    // добавить состояние поверх текущего
    void pushState(std::unique_ptr<IGameState> newState);

    // удалить верхнее состояние (или несколько)
    void popState(int count = 1);

    // безопасный возврат в редактор карт (защита от пустого стека и черного экрана)
    void returnToMapEditor(const std::string& levelFileName, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer);

    // системные методы
    void processInput(GLFWwindow* window, float dt);
    void update(float dt);
    void render();
    void resize(int width, int height);
    // применение отложенных изменений стейтов
    void applyPendingChanges();

    bool isEmpty() const { return m_states.empty(); }
};