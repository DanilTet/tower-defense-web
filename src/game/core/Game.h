#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include "states/GameStateManager.h"
#include "renderer/SpriteRenderer.h"
#include "renderer/TextRenderer.h"

class Game {
public:
	int width, height; // размеры окна, которые могут изменяться при ресайзе

	Game(int width, int height);
	~Game();

	void init();
	void processInput(GLFWwindow* window, float dt);
	void update(float dt);
	void render();
	void resize(int width, int height);

private:
	GameStateManager m_stateManager;

	std::shared_ptr<SpriteRenderer> m_renderer;
	std::unique_ptr<TextRenderer> m_textRenderer;
};