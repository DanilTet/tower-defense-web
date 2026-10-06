#include "GameStateManager.h"
#include "MapEditorState.h"
#include <iostream>

GameStateManager::~GameStateManager() {
	while (!m_states.empty()) {
		m_states.back()->cleanup();
		m_states.pop_back();
	}
}

void GameStateManager::setState(std::unique_ptr<IGameState> newState) {
	std::cout << "[GameStateManager] setState: scheduling transition to new state" << std::endl;
	m_nextState = std::move(newState);
	m_clearAllAndSet = true;
}

void GameStateManager::pushState(std::unique_ptr<IGameState> newState) {
	std::cout << "[GameStateManager] pushState: scheduling push of new state" << std::endl;
	m_nextState = std::move(newState);
	m_clearAllAndSet = false;
}

void GameStateManager::popState(int count) {
	std::cout << "[GameStateManager] popState requested (count: " << count << ")" << std::endl;
	m_popCount += count;
}

void GameStateManager::returnToMapEditor(const std::string& levelFileName, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer) {
	std::cout << "[GameStateManager] returnToMapEditor scheduled for level: " << levelFileName << std::endl;
	m_returnToEditorRequested = true;
	m_editorFallbackLevel = levelFileName;
	m_editorWidth = width;
	m_editorHeight = height;
	m_editorRenderer = renderer;
	m_editorTextRenderer = textRenderer;
}

void GameStateManager::processInput(GLFWwindow* window, float dt) {
	if (!m_states.empty()) {
		m_states.back()->processInput(window, dt);
	}
}

void GameStateManager::update(float dt) {
	if (!m_states.empty()) {
		m_states.back()->update(dt);
	}
}

void GameStateManager::render() {
	// если на паузе
	if (m_states.size() > 1) {
		// рисуем последний стейт
		m_states[m_states.size() - 2]->render();
	}

	// и тут самый верхний стейт типа паузу
	if (!m_states.empty()) {
		m_states.back()->render();
	}
}

void GameStateManager::applyPendingChanges() {
	// 1. Приоритетная обработка возврата в редактор карт
	if (m_returnToEditorRequested) {
		m_returnToEditorRequested = false;
		m_popCount = 0;

		int editorIdx = -1;
		for (int i = static_cast<int>(m_states.size()) - 1; i >= 0; --i) {
			if (dynamic_cast<MapEditorState*>(m_states[i].get()) != nullptr) {
				editorIdx = i;
				break;
			}
		}

		if (editorIdx >= 0) {
			std::cout << "[GameStateManager] Found MapEditorState at index " << editorIdx << ", unwinding stack..." << std::endl;
			while (static_cast<int>(m_states.size()) > editorIdx + 1) {
				std::cout << "[GameStateManager] Popping state to reach MapEditor, remaining: " << m_states.size() << std::endl;
				m_states.back()->cleanup();
				m_states.pop_back();
			}
		} else {
			std::cout << "[GameStateManager] MapEditorState not in stack, creating fresh instance for level: " << m_editorFallbackLevel << std::endl;
			while (!m_states.empty()) {
				m_states.back()->cleanup();
				m_states.pop_back();
			}
			m_states.push_back(std::make_unique<MapEditorState>(*this, m_editorWidth, m_editorHeight, m_editorRenderer, m_editorTextRenderer, m_editorFallbackLevel));
			m_states.back()->init();
		}
		return;
	}

	// 2. Обычный возврат из стека (защита: никогда не извлекаем последний корневой стейт в ноль!)
	while (m_popCount > 0 && m_states.size() > 1) {
		std::cout << "[GameStateManager] Popping state, remaining before pop: " << m_states.size() << std::endl;
		m_states.back()->cleanup();
		m_states.pop_back();
		m_popCount--;
	}
	m_popCount = 0;

	// 3. Отложенный стейт
	if (m_nextState) {
		if (m_clearAllAndSet) {
			std::cout << "[GameStateManager] Clearing " << m_states.size() << " existing states..." << std::endl;
			while (!m_states.empty()) {
				m_states.back()->cleanup();
				m_states.pop_back();
			}
		}
		m_states.push_back(std::move(m_nextState));
		std::cout << "[GameStateManager] Initializing new state (stack size: " << m_states.size() << ")" << std::endl;
		m_states.back()->init();

		m_clearAllAndSet = false;
	}
}

void GameStateManager::resize(int width, int height) {
	if (!m_states.empty()) {
		m_states.back()->resize(width, height);
	}
}