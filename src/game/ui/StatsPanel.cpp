#include "StatsPanel.h"
#include "core/WaveManager.h"
#include "renderer/TextRenderer.h"
#include <string>
#include "gameplay/PlayerStats.h"
#include "UICommon.h"

void StatsPanel::drawStatsPanel(const PlayerStats& stats, WaveManager* waveSize, TextRenderer* textRenderer, int screenWidth, int screenHeight) {
	// Вся статистика (HP базы, деньги, номер волны) теперь гармонично интегрирована
	// в единый верхний правый HUD (TimeControlUI) в GameplayRenderer
	(void)stats;
	(void)waveSize;
	(void)textRenderer;
	(void)screenWidth;
	(void)screenHeight;
}