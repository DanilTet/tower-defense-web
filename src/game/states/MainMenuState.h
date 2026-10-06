#pragma once
#include "IGameState.h"
#include <memory>

class GameStateManager;
class SpriteRenderer;
class TextRenderer;

class MainMenuState : public IGameState {
private:
	GameStateManager& m_stateManager;
	int m_width, m_height;
	std::shared_ptr<SpriteRenderer> m_renderer;
	TextRenderer* m_textRenderer;

	bool m_mousePressedLastFrame = false;
	bool m_suppressClickUntilRelease = true;
	bool m_ctrlShiftDWasDown = false;

	struct MenuLayout {
		float btnW;
		float btnH;
		float btnX;
		float startBtnY;
		float loadBtnY;
		float editorBtnY;
		float settingsBtnY;
		float exitBtnY;
		float btnFontScale;
		float titleFontScale;
		float titleY;
	};
	MenuLayout calculateLayout() const;

	bool isButtonClicked(double mouseX, double mouseY, float btnX, float btnY, float btnW, float btnH);
public:
	MainMenuState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer);

	void init() override;
	void cleanup() override;
	void processInput(GLFWwindow* window, float dt) override;
	void update(float dt) override;
	void render() override;
	void resize(int width, int height) override;
};