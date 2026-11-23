#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include "../core/LevelManager.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;
struct GLFWwindow;

class EditorLevelSettingsModal {
public:
    enum class FocusedType {
        None,
        Money,
        Health,
        RuleFrom,
        RuleTo,
        RuleReward,
        RuleStep,
        HpRuleFrom,
        HpRuleTo,
        HpRuleValue
    };

    struct FocusedFieldInfo {
        FocusedType type = FocusedType::None;
        std::string mobKey = "";
        int ruleIndex = -1;

        bool operator==(const FocusedFieldInfo& other) const {
            return type == other.type && mobKey == other.mobKey && ruleIndex == other.ruleIndex;
        }
        bool operator!=(const FocusedFieldInfo& other) const {
            return !(*this == other);
        }
    };

    struct EditableRule {
        std::string fromWaveStr = "1";
        std::string toWaveStr = "99";
        std::string rewardStr = "1";
        std::string rewardStepStr = "1";
    };

    struct EditableHpRule {
        std::string fromWaveStr = "1";
        std::string toWaveStr = "99";
        std::string hpStr = "18";
    };

    struct KeyState {
        bool isDown = false;
        float holdTimer = 0.0f;
        float repeatTimer = 0.0f;
    };

    EditorLevelSettingsModal() = default;

    void open(int currentMoney, int currentHealth,
              const std::vector<std::string>& allowedTowers,
              const std::unordered_map<std::string, int>& towerMaxTiers,
              const std::map<std::string, std::vector<EnemyWaveBountyRule>>& bountyRules = {},
              int totalWaves = 15,
              bool hasSidePockets = false,
              const std::map<std::string, std::vector<EnemyHpRule>>& hpRules = {});

    // Обратная совместимость со старым API open
    void open(int currentMoney, int currentHealth,
              const std::vector<std::string>& allowedTowers,
              const std::unordered_map<std::string, int>& towerMaxTiers,
              const std::map<std::string, GlobalBountyConfig>& bounties);

    void close();

    void toggle(int currentMoney, int currentHealth,
                const std::vector<std::string>& allowedTowers,
                const std::unordered_map<std::string, int>& towerMaxTiers,
                const std::map<std::string, std::vector<EnemyWaveBountyRule>>& bountyRules = {},
                int totalWaves = 15, bool hasSidePockets = false,
                const std::map<std::string, std::vector<EnemyHpRule>>& hpRules = {});

    void toggle(int currentMoney, int currentHealth,
                const std::vector<std::string>& allowedTowers,
                const std::unordered_map<std::string, int>& towerMaxTiers,
                const std::map<std::string, GlobalBountyConfig>& bounties);

    bool isOpen() const { return m_isOpen; }

    void update(float dt);

    bool handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                     bool leftReleased, bool isEscJustPressed, float dt,
                     int screenWidth, int screenHeight,
                     int& outMoney, int& outHealth,
                     std::vector<std::string>& outAllowedTowers,
                     std::unordered_map<std::string, int>& outTowerMaxTiers,
                     int& outMaxUpgradeTier,
                     std::map<std::string, std::vector<EnemyWaveBountyRule>>& outEnemyBountyRules,
                     std::map<std::string, std::vector<EnemyHpRule>>& outEnemyHpRules,
                     bool& outHasSidePockets,
                     bool& outDirty);

    bool handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                     bool leftReleased, bool isEscJustPressed, float dt,
                     int screenWidth, int screenHeight,
                     int& outMoney, int& outHealth,
                     std::vector<std::string>& outAllowedTowers,
                     std::unordered_map<std::string, int>& outTowerMaxTiers,
                     int& outMaxUpgradeTier,
                     std::map<std::string, std::vector<EnemyWaveBountyRule>>& outEnemyBountyRules,
                     bool& outHasSidePockets,
                     bool& outDirty);

    bool handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                     bool leftReleased, bool isEscJustPressed, float dt,
                     int screenWidth, int screenHeight,
                     int& outMoney, int& outHealth,
                     std::vector<std::string>& outAllowedTowers,
                     std::unordered_map<std::string, int>& outTowerMaxTiers,
                     int& outMaxUpgradeTier,
                     std::map<std::string, std::vector<EnemyWaveBountyRule>>& outEnemyBountyRules,
                     bool& outDirty);

    // Обратная совместимость со старым API handleInput
    bool handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                     bool leftReleased, bool isEscJustPressed, float dt,
                     int screenWidth, int screenHeight,
                     int& outMoney, int& outHealth,
                     std::vector<std::string>& outAllowedTowers,
                     std::unordered_map<std::string, int>& outTowerMaxTiers,
                     int& outMaxUpgradeTier,
                     std::map<std::string, GlobalBountyConfig>& outEnemyGlobalBounties,
                     bool& outDirty);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const Texture2D* whiteTexture,
                int screenWidth, int screenHeight, glm::vec2 mousePos);

private:
    bool checkKeyRepeat(GLFWwindow* window, int key, float dt);
    static bool isPointInRect(glm::vec2 p, glm::vec2 pos, glm::vec2 size);
    float calculateTotalEconHeight(float scale) const;

    bool m_isOpen = false;
    bool m_wasLeftDown = false;
    bool m_justOpened = false;

    std::string m_moneyStr = "50";
    std::string m_healthStr = "20";
    std::vector<std::string> m_allowedTowers = { "Basic", "Mercury", "Piston" };
    std::unordered_map<std::string, int> m_towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } };
    bool m_hasSidePockets = false;

    // Структура правил экономики врагов
    std::vector<std::pair<std::string, std::string>> m_mobTypes = {
        { "Coal",    "Уголёк (Coal)" },
        { "Spark",   "Искра (Spark)" },
        { "Mercury", "Ртуть (Mercury)" }
    };
    std::map<std::string, std::vector<EditableRule>> m_mobRules;
    std::map<std::string, std::vector<EditableHpRule>> m_mobHpRules;

    FocusedFieldInfo m_focused;
    float m_cursorBlinkTimer = 0.0f;
    float m_scrollOffset = 0.0f;
    int m_totalWaves = 15;

    std::unordered_map<int, KeyState> m_keyStates;
};

