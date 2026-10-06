#pragma once
#include <glm/glm.hpp>

class WaveManager;
class TextRenderer;
struct PlayerStats;

class StatsPanel {
public:
	void drawStatsPanel(const PlayerStats& stats, WaveManager* waveSize, TextRenderer* textRenderer, int screenWidth, int screenHeight);
};