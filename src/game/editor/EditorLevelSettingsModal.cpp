#include "EditorLevelSettingsModal.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include "../ui/UICommon.h"
#include "../core/InputManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <iostream>

bool EditorLevelSettingsModal::isPointInRect(glm::vec2 p, glm::vec2 pos, glm::vec2 size) {
    return p.x >= pos.x && p.x <= pos.x + size.x &&
           p.y >= pos.y && p.y <= pos.y + size.y;
}

void EditorLevelSettingsModal::open(int currentMoney, int currentHealth,
                                    const std::vector<std::string>& allowedTowers,
                                    const std::unordered_map<std::string, int>& towerMaxTiers,
                                    const std::map<std::string, std::vector<EnemyWaveBountyRule>>& bountyRules,
                                    int totalWaves, bool hasSidePockets,
                                    const std::map<std::string, std::vector<EnemyHpRule>>& hpRules)
{
    m_isOpen = true;
    m_justOpened = true;
    m_wasLeftDown = true;
    m_hasSidePockets = hasSidePockets;
    m_focused = { FocusedType::None, "", -1 };
    m_cursorBlinkTimer = 0.0f;
    m_scrollOffset = 0.0f;
    if (totalWaves > 0) {
        m_totalWaves = totalWaves;
    } else {
        m_totalWaves = 15;
    }

    m_moneyStr = std::to_string(std::max(0, currentMoney));
    m_healthStr = std::to_string(std::max(1, currentHealth));
    if (allowedTowers.empty()) {
        m_allowedTowers = std::vector<std::string>{"Basic", "Mercury", "Piston"};
    } else {
        m_allowedTowers = allowedTowers;
    }
    m_towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } };
    for (const auto& kv : towerMaxTiers) {
        m_towerMaxTiers[kv.first] = std::clamp(kv.second, 1, 3);
    }

    m_mobRules.clear();
    for (const auto& mob : m_mobTypes) {
        const std::string& key = mob.first;
        auto it = bountyRules.find(key);
        if (it != bountyRules.end() && !it->second.empty()) {
            for (const auto& r : it->second) {
                EditableRule er;
                er.fromWaveStr = std::to_string(std::max(1, r.fromWave));
                er.toWaveStr = std::to_string(std::max(1, r.toWave));
                er.rewardStr = std::to_string(std::max(0, r.reward));
                er.rewardStepStr = std::to_string(std::max(1, r.rewardStep));
                m_mobRules[key].push_back(er);
            }
        } else {
            EditableRule er;
            er.fromWaveStr = "1";
            er.toWaveStr = "99";
            er.rewardStr = "1";
            if (key == "Coal") {
                er.rewardStepStr = "2";
            } else {
                er.rewardStepStr = "1";
            }
            m_mobRules[key].push_back(er);
        }
    }

    m_mobHpRules.clear();
    for (const auto& mob : m_mobTypes) {
        const std::string& key = mob.first;
        auto it = hpRules.find(key);
        if (it != hpRules.end() && !it->second.empty()) {
            for (const auto& r : it->second) {
                EditableHpRule er;
                er.fromWaveStr = std::to_string(std::max(1, r.fromWave));
                er.toWaveStr = std::to_string(std::max(1, r.toWave));
                er.hpStr = std::to_string(std::max(1, r.hp));
                m_mobHpRules[key].push_back(er);
            }
        }
    }
}

void EditorLevelSettingsModal::open(int currentMoney, int currentHealth,
                                    const std::vector<std::string>& allowedTowers,
                                    const std::unordered_map<std::string, int>& towerMaxTiers,
                                    const std::map<std::string, GlobalBountyConfig>& bounties)
{
    std::map<std::string, std::vector<EnemyWaveBountyRule>> rules;
    for (const auto& kv : bounties) {
        rules[kv.first] = { { 1, 99, kv.second.reward, kv.second.rewardStep } };
    }
    open(currentMoney, currentHealth, allowedTowers, towerMaxTiers, rules, 15);
}

void EditorLevelSettingsModal::close() {
    m_isOpen = false;
    m_focused = { FocusedType::None, "", -1 };
    m_keyStates.clear();
}

void EditorLevelSettingsModal::toggle(int currentMoney, int currentHealth,
                                      const std::vector<std::string>& allowedTowers,
                                      const std::unordered_map<std::string, int>& towerMaxTiers,
                                      const std::map<std::string, std::vector<EnemyWaveBountyRule>>& bountyRules,
                                      int totalWaves, bool hasSidePockets,
                                      const std::map<std::string, std::vector<EnemyHpRule>>& hpRules)
{
    if (m_isOpen) {
        close();
    } else {
        open(currentMoney, currentHealth, allowedTowers, towerMaxTiers, bountyRules, totalWaves, hasSidePockets, hpRules);
    }
}

void EditorLevelSettingsModal::toggle(int currentMoney, int currentHealth,
                                      const std::vector<std::string>& allowedTowers,
                                      const std::unordered_map<std::string, int>& towerMaxTiers,
                                      const std::map<std::string, GlobalBountyConfig>& bounties)
{
    if (m_isOpen) {
        close();
    } else {
        open(currentMoney, currentHealth, allowedTowers, towerMaxTiers, bounties);
    }
}

void EditorLevelSettingsModal::update(float dt) {
    if (!m_isOpen) return;
    m_cursorBlinkTimer += dt;
    if (m_cursorBlinkTimer >= 1.0f) {
        m_cursorBlinkTimer -= 1.0f;
    }
}

bool EditorLevelSettingsModal::checkKeyRepeat(GLFWwindow* window, int key, float dt) {
    if (!window) return false;
    int state = glfwGetKey(window, key);
    KeyState& ks = m_keyStates[key];

    if (state == GLFW_PRESS) {
        if (!ks.isDown) {
            ks.isDown = true;
            ks.holdTimer = 0.0f;
            ks.repeatTimer = 0.0f;
            return true;
        } else {
            ks.holdTimer += dt;
            const float repeatDelay = 0.40f;
            const float repeatRate = 0.06f;
            if (ks.holdTimer >= repeatDelay) {
                ks.repeatTimer += dt;
                if (ks.repeatTimer >= repeatRate) {
                    ks.repeatTimer -= repeatRate;
                    return true;
                }
            }
        }
    } else {
        ks.isDown = false;
        ks.holdTimer = 0.0f;
        ks.repeatTimer = 0.0f;
    }
    return false;
}

float EditorLevelSettingsModal::calculateTotalEconHeight(float scale) const {
    float h = 24.0f * scale; // Заголовок секции экономики
    for (const auto& mob : m_mobTypes) {
        h += 24.0f * scale; // Заголовок моба и кнопка "+ Добавить волны"
        h += 4.0f * scale;  // отступ
        auto it = m_mobRules.find(mob.first);
        size_t count = 1;
        if (it != m_mobRules.end()) {
            count = it->second.size();
        }
        h += static_cast<float>(count) * (24.0f * scale + 4.0f * scale);
        h += 8.0f * scale;  // разделитель между мобами
    }

    // Секция прогрессии здоровья (HP) врагов по волнам
    h += 12.0f * scale; // разделитель между секциями
    h += 24.0f * scale; // Заголовок секции HP
    for (const auto& mob : m_mobTypes) {
        h += 24.0f * scale; // Заголовок моба и кнопка "+ Добавить волны"
        h += 4.0f * scale;  // отступ
        auto it = m_mobHpRules.find(mob.first);
        size_t count = 0;
        if (it != m_mobHpRules.end()) {
            count = it->second.size();
        }
        h += static_cast<float>(count) * (24.0f * scale + 4.0f * scale);
        h += 8.0f * scale;  // разделитель между мобами
    }
    return h;
}



bool EditorLevelSettingsModal::handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                                          bool leftReleased, bool isEscJustPressed, float dt,
                                          int screenWidth, int screenHeight,
                                          int& outMoney, int& outHealth,
                                          std::vector<std::string>& outAllowedTowers,
                                          std::unordered_map<std::string, int>& outTowerMaxTiers,
                                          int& outMaxUpgradeTier,
                                          std::map<std::string, std::vector<EnemyWaveBountyRule>>& outEnemyBountyRules,
                                          std::map<std::string, std::vector<EnemyHpRule>>& outEnemyHpRules,
                                          bool& outHasSidePockets,
                                          bool& outDirty)
{
    if (!m_isOpen) return false;

    if (isEscJustPressed || checkKeyRepeat(window, GLFW_KEY_ESCAPE, dt)) {
        close();
        return true;
    }

    float sw = static_cast<float>(screenWidth > 0 ? screenWidth : 1280);
    float sh = static_cast<float>(screenHeight > 0 ? screenHeight : 720);
    float scale = GetUIScale(static_cast<int>(sw), static_cast<int>(sh));

    float modalW = std::clamp(640.0f * scale, 320.0f, std::max(320.0f, sw - 20.0f));
    float modalH = std::clamp(620.0f * scale, 320.0f, std::max(320.0f, sh - 20.0f));
    glm::vec2 modalPos((sw - modalW) * 0.5f, (sh - modalH) * 0.5f);

    float headerH = 40.0f * scale;
    glm::vec2 crossPos(modalPos.x + modalW - 36.0f * scale, modalPos.y + 6.0f * scale);
    glm::vec2 crossSize(28.0f * scale, 28.0f * scale);

    float contentLeft = modalPos.x + 24.0f * scale;
    float contentW = modalW - 48.0f * scale;

    // Секция 1: Стартовые деньги и здоровье
    float resGap = 16.0f * scale;
    float resColW = (contentW - resGap) * 0.5f;
    float resRowY = modalPos.y + headerH + 8.0f * scale;
    float resLabelH = 16.0f * scale;
    float resBoxH = 26.0f * scale;

    glm::vec2 moneyBoxPos(contentLeft, resRowY + resLabelH);
    glm::vec2 moneyBoxSize(resColW, resBoxH);
    glm::vec2 healthBoxPos(contentLeft + resColW + resGap, resRowY + resLabelH);
    glm::vec2 healthBoxSize(resColW, resBoxH);

    // Секция 2: Разрешенные башни и тиры
    float towersLabelY = resRowY + resLabelH + resBoxH + 10.0f * scale;
    float towerRowH = 24.0f * scale;
    float rowSpacing = 4.0f * scale;
    float tierBtnW = 36.0f * scale;
    float tierBtnH = 22.0f * scale;
    float tierBtnGap = 4.0f * scale;
    float totalTierGroupW = 3.0f * tierBtnW + 2.0f * tierBtnGap;
    float rowToggleW = contentW - totalTierGroupW - 10.0f * scale;

    float rowBasicY = towersLabelY + 18.0f * scale;
    glm::vec2 rowBasicTogglePos(contentLeft, rowBasicY);
    glm::vec2 rowBasicToggleSize(rowToggleW, towerRowH);
    glm::vec2 basicT1Pos(contentLeft + contentW - totalTierGroupW, rowBasicY + (towerRowH - tierBtnH) * 0.5f);
    glm::vec2 basicT2Pos(basicT1Pos.x + tierBtnW + tierBtnGap, basicT1Pos.y);
    glm::vec2 basicT3Pos(basicT2Pos.x + tierBtnW + tierBtnGap, basicT1Pos.y);

    float rowMercuryY = rowBasicY + towerRowH + rowSpacing;
    glm::vec2 rowMercuryTogglePos(contentLeft, rowMercuryY);
    glm::vec2 rowMercuryToggleSize(rowToggleW, towerRowH);
    glm::vec2 mercuryT1Pos(contentLeft + contentW - totalTierGroupW, rowMercuryY + (towerRowH - tierBtnH) * 0.5f);
    glm::vec2 mercuryT2Pos(mercuryT1Pos.x + tierBtnW + tierBtnGap, mercuryT1Pos.y);
    glm::vec2 mercuryT3Pos(mercuryT2Pos.x + tierBtnW + tierBtnGap, mercuryT1Pos.y);

    float rowPistonY = rowMercuryY + towerRowH + rowSpacing;
    glm::vec2 rowPistonTogglePos(contentLeft, rowPistonY);
    glm::vec2 rowPistonToggleSize(rowToggleW, towerRowH);
    glm::vec2 pistonT1Pos(contentLeft + contentW - totalTierGroupW, rowPistonY + (towerRowH - tierBtnH) * 0.5f);
    glm::vec2 pistonT2Pos(pistonT1Pos.x + tierBtnW + tierBtnGap, pistonT1Pos.y);
    glm::vec2 pistonT3Pos(pistonT2Pos.x + tierBtnW + tierBtnGap, pistonT1Pos.y);

    glm::vec2 tierBtnSize(tierBtnW, tierBtnH);

    // Секция 4: Нижние кнопки [ Сохранить ] и [ Отмена ]
    float btnH = 32.0f * scale;
    float btnY = modalPos.y + modalH - btnH - 12.0f * scale;
    float saveBtnW = 150.0f * scale;
    float cancelBtnW = 120.0f * scale;
    float btnGap = 20.0f * scale;
    float totalBtnW = saveBtnW + cancelBtnW + btnGap;
    glm::vec2 btnSavePos(modalPos.x + (modalW - totalBtnW) * 0.5f, btnY);
    glm::vec2 btnSaveSize(saveBtnW, btnH);
    glm::vec2 btnCancelPos(btnSavePos.x + saveBtnW + btnGap, btnY);
    glm::vec2 btnCancelSize(cancelBtnW, btnH);

    // Секция 3: Контейнер экономики (скроллируемая область)
    float rowPocketsY = rowPistonY + towerRowH + rowSpacing + 4.0f * scale;
    glm::vec2 rowPocketsTogglePos(contentLeft, rowPocketsY);
    glm::vec2 rowPocketsToggleSize(contentW, towerRowH);

    float econAreaTop = rowPocketsY + towerRowH + 8.0f * scale;
    float econAreaBottom = btnY - 10.0f * scale;
    float econAreaH = econAreaBottom - econAreaTop;
    glm::vec2 econAreaPos(contentLeft, econAreaTop);
    glm::vec2 econAreaSize(contentW, econAreaH);

    float totalEconH = calculateTotalEconHeight(scale);
    float maxScroll = std::max(0.0f, totalEconH - econAreaH);
    m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, maxScroll);

    // 1. Скролл колесиком мыши
    float scrollY = InputManager::getScrollY();
    if (std::abs(scrollY) > 0.01f) {
        if (isPointInRect(mousePos, econAreaPos, econAreaSize)) {
            m_scrollOffset -= scrollY * 26.0f * scale;
            m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, maxScroll);
            InputManager::clearScroll();
            return true;
        }
    }

    // 2. Клавиатурный фокус и Tab
    if (checkKeyRepeat(window, GLFW_KEY_TAB, dt)) {
        struct FocusTarget {
            FocusedType type;
            std::string mobKey;
            int ruleIndex;
        };
        std::vector<FocusTarget> targets;
        targets.push_back({ FocusedType::Money, "", -1 });
        targets.push_back({ FocusedType::Health, "", -1 });
        for (const auto& mob : m_mobTypes) {
            auto it = m_mobRules.find(mob.first);
            if (it != m_mobRules.end()) {
                for (int i = 0; i < static_cast<int>(it->second.size()); ++i) {
                    targets.push_back({ FocusedType::RuleFrom, mob.first, i });
                    targets.push_back({ FocusedType::RuleTo, mob.first, i });
                    targets.push_back({ FocusedType::RuleReward, mob.first, i });
                    targets.push_back({ FocusedType::RuleStep, mob.first, i });
                }
            }
        }
        for (const auto& mob : m_mobTypes) {
            auto it = m_mobHpRules.find(mob.first);
            if (it != m_mobHpRules.end()) {
                for (int i = 0; i < static_cast<int>(it->second.size()); ++i) {
                    targets.push_back({ FocusedType::HpRuleFrom, mob.first, i });
                    targets.push_back({ FocusedType::HpRuleTo, mob.first, i });
                    targets.push_back({ FocusedType::HpRuleValue, mob.first, i });
                }
            }
        }

        int curIdx = -1;
        for (size_t i = 0; i < targets.size(); ++i) {
            if (targets[i].type == m_focused.type && targets[i].mobKey == m_focused.mobKey && targets[i].ruleIndex == m_focused.ruleIndex) {
                curIdx = static_cast<int>(i);
                break;
            }
        }

        int nextIdx = (curIdx + 1) % static_cast<int>(targets.size());
        m_focused = { targets[nextIdx].type, targets[nextIdx].mobKey, targets[nextIdx].ruleIndex };
        m_cursorBlinkTimer = 0.0f;
    }

    // 3. Сохранение по Enter
    auto commitAndSave = [&]() {
        int mVal = 50;
        try { if (!m_moneyStr.empty()) mVal = std::max(0, std::stoi(m_moneyStr)); } catch (...) { mVal = 50; }
        int hVal = 20;
        try { if (!m_healthStr.empty()) hVal = std::max(1, std::stoi(m_healthStr)); } catch (...) { hVal = 20; }

        if (m_allowedTowers.empty()) {
            m_allowedTowers = { "Basic", "Mercury", "Piston" };
        }

        outMoney = mVal;
        outHealth = hVal;
        outAllowedTowers = m_allowedTowers;
        outTowerMaxTiers = m_towerMaxTiers;
        int maxT = 1;
        for (const auto& kv : m_towerMaxTiers) {
            maxT = std::max(maxT, kv.second);
        }
        outMaxUpgradeTier = maxT;

        outEnemyBountyRules.clear();
        for (const auto& mob : m_mobTypes) {
            std::vector<EnemyWaveBountyRule> rules;
            auto it = m_mobRules.find(mob.first);
            if (it != m_mobRules.end()) {
                for (const auto& er : it->second) {
                    EnemyWaveBountyRule r;
                    try { r.fromWave = er.fromWaveStr.empty() ? 1 : std::max(1, std::stoi(er.fromWaveStr)); } catch (...) { r.fromWave = 1; }
                    try { r.toWave = er.toWaveStr.empty() ? 99 : std::max(r.fromWave, std::stoi(er.toWaveStr)); } catch (...) { r.toWave = 99; }
                    try { r.reward = er.rewardStr.empty() ? 1 : std::max(0, std::stoi(er.rewardStr)); } catch (...) { r.reward = 1; }
                    try { r.rewardStep = er.rewardStepStr.empty() ? 1 : std::max(1, std::stoi(er.rewardStepStr)); } catch (...) { r.rewardStep = 1; }
                    rules.push_back(r);
                }
            }
            if (rules.empty()) {
                rules.push_back({ 1, 99, 1, 1 });
            }
            outEnemyBountyRules[mob.first] = rules;
        }

        outEnemyHpRules.clear();
        for (const auto& mob : m_mobTypes) {
            std::vector<EnemyHpRule> hpRules;
            auto it = m_mobHpRules.find(mob.first);
            if (it != m_mobHpRules.end()) {
                for (const auto& er : it->second) {
                    EnemyHpRule r;
                    try {
                        if (er.fromWaveStr.empty()) {
                            r.fromWave = 1;
                        } else {
                            r.fromWave = std::max(1, std::stoi(er.fromWaveStr));
                        }
                    } catch (...) {
                        r.fromWave = 1;
                    }
                    try {
                        if (er.toWaveStr.empty()) {
                            r.toWave = 99;
                        } else {
                            r.toWave = std::max(r.fromWave, std::stoi(er.toWaveStr));
                        }
                    } catch (...) {
                        r.toWave = 99;
                    }
                    try {
                        if (er.hpStr.empty()) {
                            r.hp = 1;
                        } else {
                            r.hp = std::max(1, std::stoi(er.hpStr));
                        }
                    } catch (...) {
                        r.hp = 1;
                    }
                    hpRules.push_back(r);
                }
            }
            if (!hpRules.empty()) {
                outEnemyHpRules[mob.first] = hpRules;
            }
        }

        outHasSidePockets = m_hasSidePockets;
        outDirty = true;
        close();
    };

    if (checkKeyRepeat(window, GLFW_KEY_ENTER, dt) || checkKeyRepeat(window, GLFW_KEY_KP_ENTER, dt)) {
        commitAndSave();
        return true;
    }

    // 4. Текстовый ввод для активного поля
    if (m_focused.type != FocusedType::None) {
        std::string* targetStr = nullptr;
        size_t maxLen = 4;
        switch (m_focused.type) {
            case FocusedType::Money:
                targetStr = &m_moneyStr; maxLen = 7; break;
            case FocusedType::Health:
                targetStr = &m_healthStr; maxLen = 4; break;
            case FocusedType::RuleFrom:
                if (m_mobRules.count(m_focused.mobKey) && m_focused.ruleIndex < static_cast<int>(m_mobRules[m_focused.mobKey].size())) {
                    targetStr = &m_mobRules[m_focused.mobKey][m_focused.ruleIndex].fromWaveStr;
                    maxLen = 3;
                }
                break;
            case FocusedType::RuleTo:
                if (m_mobRules.count(m_focused.mobKey) && m_focused.ruleIndex < static_cast<int>(m_mobRules[m_focused.mobKey].size())) {
                    targetStr = &m_mobRules[m_focused.mobKey][m_focused.ruleIndex].toWaveStr;
                    maxLen = 3;
                }
                break;
            case FocusedType::RuleReward:
                if (m_mobRules.count(m_focused.mobKey) && m_focused.ruleIndex < static_cast<int>(m_mobRules[m_focused.mobKey].size())) {
                    targetStr = &m_mobRules[m_focused.mobKey][m_focused.ruleIndex].rewardStr;
                    maxLen = 4;
                }
                break;
            case FocusedType::RuleStep:
                if (m_mobRules.count(m_focused.mobKey) && m_focused.ruleIndex < static_cast<int>(m_mobRules[m_focused.mobKey].size())) {
                    targetStr = &m_mobRules[m_focused.mobKey][m_focused.ruleIndex].rewardStepStr;
                    maxLen = 3;
                }
                break;
            case FocusedType::HpRuleFrom:
                if (m_mobHpRules.count(m_focused.mobKey) && m_focused.ruleIndex < static_cast<int>(m_mobHpRules[m_focused.mobKey].size())) {
                    targetStr = &m_mobHpRules[m_focused.mobKey][m_focused.ruleIndex].fromWaveStr;
                    maxLen = 3;
                }
                break;
            case FocusedType::HpRuleTo:
                if (m_mobHpRules.count(m_focused.mobKey) && m_focused.ruleIndex < static_cast<int>(m_mobHpRules[m_focused.mobKey].size())) {
                    targetStr = &m_mobHpRules[m_focused.mobKey][m_focused.ruleIndex].toWaveStr;
                    maxLen = 3;
                }
                break;
            case FocusedType::HpRuleValue:
                if (m_mobHpRules.count(m_focused.mobKey) && m_focused.ruleIndex < static_cast<int>(m_mobHpRules[m_focused.mobKey].size())) {
                    targetStr = &m_mobHpRules[m_focused.mobKey][m_focused.ruleIndex].hpStr;
                    maxLen = 5;
                }
                break;
            default: break;
        }

        if (targetStr) {
            if (checkKeyRepeat(window, GLFW_KEY_BACKSPACE, dt)) {
                if (!targetStr->empty()) {
                    targetStr->pop_back();
                    m_cursorBlinkTimer = 0.0f;
                }
            }

            for (int d = 0; d <= 9; ++d) {
                if (checkKeyRepeat(window, GLFW_KEY_0 + d, dt) || checkKeyRepeat(window, GLFW_KEY_KP_0 + d, dt)) {
                    if (*targetStr == "0") {
                        targetStr->clear();
                    }
                    if (targetStr->length() < maxLen) {
                        *targetStr += std::to_string(d);
                        m_cursorBlinkTimer = 0.0f;
                    }
                }
            }
        }
    }

    // 5. Обработка клика мыши
    if (m_justOpened) {
        if (!leftDown) {
            m_justOpened = false;
        }
        m_wasLeftDown = leftDown;
        return true;
    }

    bool lmbJustPressed = leftDown && !m_wasLeftDown;
    m_wasLeftDown = leftDown;

    if (!lmbJustPressed) {
        return true;
    }

    // Крестик
    if (isPointInRect(mousePos, crossPos, crossSize)) {
        close();
        return true;
    }

    // Отмена
    if (isPointInRect(mousePos, btnCancelPos, btnCancelSize)) {
        close();
        return true;
    }

    // Сохранить
    if (isPointInRect(mousePos, btnSavePos, btnSaveSize)) {
        commitAndSave();
        return true;
    }

    // Фокус полей ресурсов
    if (isPointInRect(mousePos, moneyBoxPos, moneyBoxSize)) {
        m_focused = { FocusedType::Money, "", -1 };
        m_cursorBlinkTimer = 0.0f;
        return true;
    }

    if (isPointInRect(mousePos, healthBoxPos, healthBoxSize)) {
        m_focused = { FocusedType::Health, "", -1 };
        m_cursorBlinkTimer = 0.0f;
        return true;
    }

    // Переключение чекбоксов башен
    auto toggleTower = [this](const std::string& type) {
        auto it = std::find(m_allowedTowers.begin(), m_allowedTowers.end(), type);
        if (it != m_allowedTowers.end()) {
            if (m_allowedTowers.size() > 1) {
                m_allowedTowers.erase(it);
            }
        } else {
            m_allowedTowers.push_back(type);
        }
    };

    auto setTowerTier = [this](const std::string& type, int tier) {
        m_towerMaxTiers[type] = tier;
    };

    if (isPointInRect(mousePos, rowBasicTogglePos, rowBasicToggleSize)) {
        toggleTower("Basic");
        m_focused = { FocusedType::None, "", -1 };
        return true;
    }
    if (isPointInRect(mousePos, basicT1Pos, tierBtnSize)) { setTowerTier("Basic", 1); m_focused = { FocusedType::None, "", -1 }; return true; }
    if (isPointInRect(mousePos, basicT2Pos, tierBtnSize)) { setTowerTier("Basic", 2); m_focused = { FocusedType::None, "", -1 }; return true; }
    if (isPointInRect(mousePos, basicT3Pos, tierBtnSize)) { setTowerTier("Basic", 3); m_focused = { FocusedType::None, "", -1 }; return true; }

    if (isPointInRect(mousePos, rowMercuryTogglePos, rowMercuryToggleSize)) {
        toggleTower("Mercury");
        m_focused = { FocusedType::None, "", -1 };
        return true;
    }
    if (isPointInRect(mousePos, mercuryT1Pos, tierBtnSize)) { setTowerTier("Mercury", 1); m_focused = { FocusedType::None, "", -1 }; return true; }
    if (isPointInRect(mousePos, mercuryT2Pos, tierBtnSize)) { setTowerTier("Mercury", 2); m_focused = { FocusedType::None, "", -1 }; return true; }
    if (isPointInRect(mousePos, mercuryT3Pos, tierBtnSize)) { setTowerTier("Mercury", 3); m_focused = { FocusedType::None, "", -1 }; return true; }

    if (isPointInRect(mousePos, rowPistonTogglePos, rowPistonToggleSize)) {
        toggleTower("Piston");
        m_focused = { FocusedType::None, "", -1 };
        return true;
    }
    if (isPointInRect(mousePos, pistonT1Pos, tierBtnSize)) { setTowerTier("Piston", 1); m_focused = { FocusedType::None, "", -1 }; return true; }
    if (isPointInRect(mousePos, pistonT2Pos, tierBtnSize)) { setTowerTier("Piston", 2); m_focused = { FocusedType::None, "", -1 }; return true; }
    if (isPointInRect(mousePos, pistonT3Pos, tierBtnSize)) { setTowerTier("Piston", 3); m_focused = { FocusedType::None, "", -1 }; return true; }

    if (isPointInRect(mousePos, rowPocketsTogglePos, rowPocketsToggleSize)) {
        m_hasSidePockets = !m_hasSidePockets;
        m_focused = { FocusedType::None, "", -1 };
        return true;
    }

    // Клик внутри области экономики
    if (isPointInRect(mousePos, econAreaPos, econAreaSize)) {
        float curY = econAreaTop + 24.0f * scale - m_scrollOffset;

        for (const auto& mob : m_mobTypes) {
            const std::string& key = mob.first;
            float headerRowY = curY;

            // Кнопка [+ Добавить волны]
            glm::vec2 addBtnPos(contentLeft + 150.0f * scale, headerRowY);
            glm::vec2 addBtnSize(135.0f * scale, 22.0f * scale);

            if (isPointInRect(mousePos, addBtnPos, addBtnSize)) {
                auto& rules = m_mobRules[key];
                int prevTo = 0;
                if (!rules.empty()) {
                    try { prevTo = std::stoi(rules.back().toWaveStr); } catch (...) { prevTo = 1; }
                }
                int newFrom = prevTo + 1;
                int newTo = std::max(newFrom, m_totalWaves > 0 ? m_totalWaves : 15);
                rules.push_back({
                    std::to_string(newFrom),
                    std::to_string(newTo),
                    "1",
                    "1"
                });
                m_focused = { FocusedType::RuleFrom, key, static_cast<int>(rules.size() - 1) };
                m_cursorBlinkTimer = 0.0f;

                float newTotalH = calculateTotalEconHeight(scale);
                float newMaxScroll = std::max(0.0f, newTotalH - econAreaH);
                m_scrollOffset = newMaxScroll;
                return true;
            }

            curY += 24.0f * scale + 4.0f * scale;

            // Список правил для этого моба
            auto& rules = m_mobRules[key];
            float ruleRowH = 24.0f * scale;
            float boxH = 22.0f * scale;

            for (int rIdx = 0; rIdx < static_cast<int>(rules.size()); ++rIdx) {
                float rowY = curY;
                float boxY = rowY + (ruleRowH - boxH) * 0.5f;

                float startX = contentLeft + 10.0f * scale;
                // Волны с: [input]
                float fromBoxX = startX + 62.0f * scale;
                glm::vec2 fromBoxPos(fromBoxX, boxY);
                glm::vec2 fromBoxSize(38.0f * scale, boxH);

                // по: [input]
                float toBoxX = fromBoxPos.x + fromBoxSize.x + 30.0f * scale;
                glm::vec2 toBoxPos(toBoxX, boxY);
                glm::vec2 toBoxSize(38.0f * scale, boxH);

                // Награда ($): [input]
                float rewBoxX = toBoxPos.x + toBoxSize.x + 88.0f * scale;
                glm::vec2 rewBoxPos(rewBoxX, boxY);
                glm::vec2 rewBoxSize(40.0f * scale, boxH);

                // Каждый N-й: [input]
                float stepBoxX = rewBoxPos.x + rewBoxSize.x + 90.0f * scale;
                glm::vec2 stepBoxPos(stepBoxX, boxY);
                glm::vec2 stepBoxSize(36.0f * scale, boxH);

                // [X]
                float delBoxX = stepBoxPos.x + stepBoxSize.x + 12.0f * scale;
                glm::vec2 delBoxPos(delBoxX, boxY);
                glm::vec2 delBoxSize(22.0f * scale, boxH);

                if (isPointInRect(mousePos, fromBoxPos, fromBoxSize)) {
                    m_focused = { FocusedType::RuleFrom, key, rIdx };
                    m_cursorBlinkTimer = 0.0f;
                    return true;
                }
                if (isPointInRect(mousePos, toBoxPos, toBoxSize)) {
                    m_focused = { FocusedType::RuleTo, key, rIdx };
                    m_cursorBlinkTimer = 0.0f;
                    return true;
                }
                if (isPointInRect(mousePos, rewBoxPos, rewBoxSize)) {
                    m_focused = { FocusedType::RuleReward, key, rIdx };
                    m_cursorBlinkTimer = 0.0f;
                    return true;
                }
                if (isPointInRect(mousePos, stepBoxPos, stepBoxSize)) {
                    m_focused = { FocusedType::RuleStep, key, rIdx };
                    m_cursorBlinkTimer = 0.0f;
                    return true;
                }
                if (isPointInRect(mousePos, delBoxPos, delBoxSize)) {
                    if (rules.size() > 1) {
                        rules.erase(rules.begin() + rIdx);
                        if (m_focused.mobKey == key && m_focused.ruleIndex == rIdx) {
                            m_focused = { FocusedType::None, "", -1 };
                        } else if (m_focused.mobKey == key && m_focused.ruleIndex > rIdx) {
                            m_focused.ruleIndex--;
                        }
                    } else if (rules.size() == 1) {
                        rules[0] = { "1", "99", "1", "1" };
                    }
                    return true;
                }

                curY += ruleRowH + 4.0f * scale;
            }

            curY += 8.0f * scale;
        }

        // Секция прогрессии здоровья врагов (HP) по волнам
        curY += 12.0f * scale + 24.0f * scale;

        for (const auto& mob : m_mobTypes) {
            const std::string& key = mob.first;
            float headerRowY = curY;

            // Кнопка [+ Добавить волны]
            glm::vec2 addBtnPos(contentLeft + 150.0f * scale, headerRowY);
            glm::vec2 addBtnSize(135.0f * scale, 22.0f * scale);

            if (isPointInRect(mousePos, addBtnPos, addBtnSize)) {
                auto& hpRules = m_mobHpRules[key];
                int prevTo = 0;
                std::string defaultHpStr = "18";
                if (key == "Spark") {
                    defaultHpStr = "6";
                } else if (key == "Mercury") {
                    defaultHpStr = "40";
                }
                if (!hpRules.empty()) {
                    try { prevTo = std::stoi(hpRules.back().toWaveStr); } catch (...) { prevTo = 1; }
                    if (!hpRules.back().hpStr.empty()) {
                        defaultHpStr = hpRules.back().hpStr;
                    }
                }
                int newFrom = prevTo + 1;
                int waveLimit = 15;
                if (m_totalWaves > 0) {
                    waveLimit = m_totalWaves;
                }
                int newTo = std::max(newFrom, waveLimit);
                hpRules.push_back({
                    std::to_string(newFrom),
                    std::to_string(newTo),
                    defaultHpStr
                });
                m_focused = { FocusedType::HpRuleFrom, key, static_cast<int>(hpRules.size() - 1) };
                m_cursorBlinkTimer = 0.0f;

                float newTotalH = calculateTotalEconHeight(scale);
                float newMaxScroll = std::max(0.0f, newTotalH - econAreaH);
                m_scrollOffset = newMaxScroll;
                return true;
            }

            curY += 24.0f * scale + 4.0f * scale;

            auto& hpRules = m_mobHpRules[key];
            float ruleRowH = 24.0f * scale;
            float boxH = 22.0f * scale;

            for (int rIdx = 0; rIdx < static_cast<int>(hpRules.size()); ++rIdx) {
                float rowY = curY;
                float boxY = rowY + (ruleRowH - boxH) * 0.5f;

                float startX = contentLeft + 10.0f * scale;
                // Волны с: [input]
                float fromBoxX = startX + 62.0f * scale;
                glm::vec2 fromBoxPos(fromBoxX, boxY);
                glm::vec2 fromBoxSize(38.0f * scale, boxH);

                // по: [input]
                float toBoxX = fromBoxPos.x + fromBoxSize.x + 30.0f * scale;
                glm::vec2 toBoxPos(toBoxX, boxY);
                glm::vec2 toBoxSize(38.0f * scale, boxH);

                // HP: [input]
                float hpBoxX = toBoxPos.x + toBoxSize.x + 38.0f * scale;
                glm::vec2 hpBoxPos(hpBoxX, boxY);
                glm::vec2 hpBoxSize(56.0f * scale, boxH);

                // [X]
                float delBoxX = hpBoxPos.x + hpBoxSize.x + 12.0f * scale;
                glm::vec2 delBoxPos(delBoxX, boxY);
                glm::vec2 delBoxSize(22.0f * scale, boxH);

                if (isPointInRect(mousePos, fromBoxPos, fromBoxSize)) {
                    m_focused = { FocusedType::HpRuleFrom, key, rIdx };
                    m_cursorBlinkTimer = 0.0f;
                    return true;
                }
                if (isPointInRect(mousePos, toBoxPos, toBoxSize)) {
                    m_focused = { FocusedType::HpRuleTo, key, rIdx };
                    m_cursorBlinkTimer = 0.0f;
                    return true;
                }
                if (isPointInRect(mousePos, hpBoxPos, hpBoxSize)) {
                    m_focused = { FocusedType::HpRuleValue, key, rIdx };
                    m_cursorBlinkTimer = 0.0f;
                    return true;
                }
                if (isPointInRect(mousePos, delBoxPos, delBoxSize)) {
                    hpRules.erase(hpRules.begin() + rIdx);
                    bool isHpFocus = (m_focused.type == FocusedType::HpRuleFrom ||
                                      m_focused.type == FocusedType::HpRuleTo ||
                                      m_focused.type == FocusedType::HpRuleValue);
                    if (isHpFocus && m_focused.mobKey == key) {
                        if (m_focused.ruleIndex == rIdx) {
                            m_focused = { FocusedType::None, "", -1 };
                        } else if (m_focused.ruleIndex > rIdx) {
                            m_focused.ruleIndex--;
                        }
                    }
                    float newTotalH = calculateTotalEconHeight(scale);
                    float newMaxScroll = std::max(0.0f, newTotalH - econAreaH);
                    m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, newMaxScroll);
                    return true;
                }

                curY += ruleRowH + 4.0f * scale;
            }

            curY += 8.0f * scale;
        }

        m_focused = { FocusedType::None, "", -1 };
        return true;
    }

    // Клик внутри окна сбрасывает фокус с полей ввода
    if (isPointInRect(mousePos, modalPos, glm::vec2(modalW, modalH))) {
        m_focused = { FocusedType::None, "", -1 };
        return true;
    }

    // Клик вне окна закрывает его
    close();
    return true;
}

bool EditorLevelSettingsModal::handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                                          bool leftReleased, bool isEscJustPressed, float dt,
                                          int screenWidth, int screenHeight,
                                          int& outMoney, int& outHealth,
                                          std::vector<std::string>& outAllowedTowers,
                                          std::unordered_map<std::string, int>& outTowerMaxTiers,
                                          int& outMaxUpgradeTier,
                                          std::map<std::string, std::vector<EnemyWaveBountyRule>>& outEnemyBountyRules,
                                          bool& outHasSidePockets,
                                          bool& outDirty)
{
    std::map<std::string, std::vector<EnemyHpRule>> dummyHpRules;
    return handleInput(window, mousePos, leftDown, leftReleased, isEscJustPressed, dt,
                       screenWidth, screenHeight, outMoney, outHealth, outAllowedTowers,
                       outTowerMaxTiers, outMaxUpgradeTier, outEnemyBountyRules, dummyHpRules,
                       outHasSidePockets, outDirty);
}

bool EditorLevelSettingsModal::handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                                          bool leftReleased, bool isEscJustPressed, float dt,
                                          int screenWidth, int screenHeight,
                                          int& outMoney, int& outHealth,
                                          std::vector<std::string>& outAllowedTowers,
                                          std::unordered_map<std::string, int>& outTowerMaxTiers,
                                          int& outMaxUpgradeTier,
                                          std::map<std::string, std::vector<EnemyWaveBountyRule>>& outEnemyBountyRules,
                                          bool& outDirty)
{
    bool dummyPockets = false;
    return handleInput(window, mousePos, leftDown, leftReleased, isEscJustPressed, dt,
                       screenWidth, screenHeight, outMoney, outHealth, outAllowedTowers,
                       outTowerMaxTiers, outMaxUpgradeTier, outEnemyBountyRules, dummyPockets, outDirty);
}



bool EditorLevelSettingsModal::handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                                          bool leftReleased, bool isEscJustPressed, float dt,
                                          int screenWidth, int screenHeight,
                                          int& outMoney, int& outHealth,
                                          std::vector<std::string>& outAllowedTowers,
                                          std::unordered_map<std::string, int>& outTowerMaxTiers,
                                          int& outMaxUpgradeTier,
                                          std::map<std::string, GlobalBountyConfig>& outEnemyGlobalBounties,
                                          bool& outDirty)
{
    std::map<std::string, std::vector<EnemyWaveBountyRule>> rules;
    bool res = handleInput(window, mousePos, leftDown, leftReleased, isEscJustPressed, dt,
                           screenWidth, screenHeight, outMoney, outHealth, outAllowedTowers,
                           outTowerMaxTiers, outMaxUpgradeTier, rules, outDirty);
    if (res && outDirty) {
        outEnemyGlobalBounties.clear();
        for (const auto& kv : rules) {
            if (!kv.second.empty()) {
                outEnemyGlobalBounties[kv.first] = { kv.second.front().reward, kv.second.front().rewardStep };
            }
        }
    }
    return res;
}

void EditorLevelSettingsModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                     const Texture2D* whiteTexture,
                                     int screenWidth, int screenHeight, glm::vec2 mousePos)
{
    if (!m_isOpen || !renderer || !whiteTexture) return;

    float sw = static_cast<float>(screenWidth > 0 ? screenWidth : 1280);
    float sh = static_cast<float>(screenHeight > 0 ? screenHeight : 720);
    float scale = GetUIScale(static_cast<int>(sw), static_cast<int>(sh));

    // 1. Затемнение фона
    renderer->drawSprite(whiteTexture, glm::vec2(0.0f, 0.0f), glm::vec2(sw, sh), 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));

    // 2. Геометрия модального окна
    float modalW = std::clamp(640.0f * scale, 320.0f, std::max(320.0f, sw - 20.0f));
    float modalH = std::clamp(620.0f * scale, 320.0f, std::max(320.0f, sh - 20.0f));
    glm::vec2 modalPos((sw - modalW) * 0.5f, (sh - modalH) * 0.5f);

    float headerH = 40.0f * scale;
    glm::vec2 crossPos(modalPos.x + modalW - 36.0f * scale, modalPos.y + 6.0f * scale);
    glm::vec2 crossSize(28.0f * scale, 28.0f * scale);

    // Рамка и фон окна
    renderer->drawSprite(whiteTexture, modalPos, glm::vec2(modalW, modalH), 0.0f, glm::vec3(0.35f, 0.42f, 0.55f));
    renderer->drawSprite(whiteTexture, modalPos + glm::vec2(2.0f), glm::vec2(modalW - 4.0f, modalH - 4.0f), 0.0f, glm::vec3(0.12f, 0.14f, 0.18f));

    // Шапка
    renderer->drawSprite(whiteTexture, modalPos, glm::vec2(modalW, headerH), 0.0f, glm::vec3(0.16f, 0.19f, 0.25f));
    renderer->drawSprite(whiteTexture, glm::vec2(modalPos.x, modalPos.y + headerH - 1.5f), glm::vec2(modalW, 1.5f), 0.0f, glm::vec3(0.28f, 0.35f, 0.48f));

    // Крестик закрытия
    bool crossHover = isPointInRect(mousePos, crossPos, crossSize);
    renderer->drawSprite(whiteTexture, crossPos, crossSize, 0.0f, crossHover ? glm::vec3(0.85f, 0.25f, 0.25f) : glm::vec3(0.22f, 0.26f, 0.34f));
    renderer->drawSprite(whiteTexture, crossPos + glm::vec2(1.5f), crossSize - glm::vec2(3.0f), 0.0f, crossHover ? glm::vec3(0.65f, 0.18f, 0.18f) : glm::vec3(0.14f, 0.17f, 0.22f));

    // Контентные границы
    float contentLeft = modalPos.x + 24.0f * scale;
    float contentW = modalW - 48.0f * scale;

    // Секция 1: Ресурсы (Деньги / Здоровье)
    float resGap = 16.0f * scale;
    float resColW = (contentW - resGap) * 0.5f;
    float resRowY = modalPos.y + headerH + 8.0f * scale;
    float resLabelH = 16.0f * scale;
    float resBoxH = 26.0f * scale;

    glm::vec2 moneyBoxPos(contentLeft, resRowY + resLabelH);
    glm::vec2 moneyBoxSize(resColW, resBoxH);
    glm::vec2 healthBoxPos(contentLeft + resColW + resGap, resRowY + resLabelH);
    glm::vec2 healthBoxSize(resColW, resBoxH);

    auto renderInputBox = [&](glm::vec2 pos, glm::vec2 size, bool focused) {
        glm::vec3 border = focused ? glm::vec3(1.0f, 0.84f, 0.25f) : glm::vec3(0.28f, 0.34f, 0.46f);
        renderer->drawSprite(whiteTexture, pos, size, 0.0f, border);
        renderer->drawSprite(whiteTexture, pos + glm::vec2(1.5f), size - glm::vec2(3.0f), 0.0f, focused ? glm::vec3(0.09f, 0.12f, 0.16f) : glm::vec3(0.06f, 0.08f, 0.11f));
    };

    renderInputBox(moneyBoxPos, moneyBoxSize, m_focused.type == FocusedType::Money);
    renderInputBox(healthBoxPos, healthBoxSize, m_focused.type == FocusedType::Health);

    // Секция 2: Разрешенные башни и тиры
    float towersLabelY = resRowY + resLabelH + resBoxH + 10.0f * scale;
    float towerRowH = 24.0f * scale;
    float rowSpacing = 4.0f * scale;
    float tierBtnW = 36.0f * scale;
    float tierBtnH = 22.0f * scale;
    float tierBtnGap = 4.0f * scale;
    float totalTierGroupW = 3.0f * tierBtnW + 2.0f * tierBtnGap;
    float rowToggleW = contentW - totalTierGroupW - 10.0f * scale;

    float rowBasicY = towersLabelY + 18.0f * scale;
    float rowMercuryY = rowBasicY + towerRowH + rowSpacing;
    float rowPistonY = rowMercuryY + towerRowH + rowSpacing;

    glm::vec2 tierBtnSize(tierBtnW, tierBtnH);

    auto renderCheckbox = [&](glm::vec2 pos, bool checked) {
        float sz = 16.0f * scale;
        glm::vec2 cbPos(pos.x, pos.y);
        glm::vec2 cbSize(sz, sz);
        renderer->drawSprite(whiteTexture, cbPos, cbSize, 0.0f, glm::vec3(0.35f, 0.45f, 0.60f));
        renderer->drawSprite(whiteTexture, cbPos + glm::vec2(1.5f), cbSize - glm::vec2(3.0f), 0.0f, glm::vec3(0.08f, 0.10f, 0.14f));
        if (checked) {
            renderer->drawSprite(whiteTexture, cbPos + glm::vec2(3.5f * scale), cbSize - glm::vec2(7.0f * scale), 0.0f, glm::vec3(0.25f, 0.85f, 0.40f));
        }
    };

    auto renderRowTierButtons = [&](const std::string& type, float rowY) {
        int maxTier = 3;
        auto it = m_towerMaxTiers.find(type);
        if (it != m_towerMaxTiers.end()) maxTier = it->second;

        float startX = contentLeft + contentW - totalTierGroupW;
        float btnY = rowY + (towerRowH - tierBtnH) * 0.5f;

        for (int t = 1; t <= 3; ++t) {
            glm::vec2 pos(startX + static_cast<float>(t - 1) * (tierBtnW + tierBtnGap), btnY);
            bool active = (maxTier == t);
            bool hover = isPointInRect(mousePos, pos, tierBtnSize);

            glm::vec3 border = active ? glm::vec3(0.95f, 0.75f, 0.20f) : (hover ? glm::vec3(0.50f, 0.60f, 0.75f) : glm::vec3(0.28f, 0.34f, 0.46f));
            glm::vec3 bg = active ? glm::vec3(0.35f, 0.28f, 0.10f) : (hover ? glm::vec3(0.20f, 0.25f, 0.34f) : glm::vec3(0.12f, 0.15f, 0.20f));

            renderer->drawSprite(whiteTexture, pos, tierBtnSize, 0.0f, border);
            renderer->drawSprite(whiteTexture, pos + glm::vec2(1.5f), tierBtnSize - glm::vec2(3.0f), 0.0f, bg);
        }
    };

    auto hasTower = [this](const std::string& type) {
        return std::find(m_allowedTowers.begin(), m_allowedTowers.end(), type) != m_allowedTowers.end();
    };

    float cbSize = 16.0f * scale;
    renderCheckbox(glm::vec2(contentLeft + 4.0f * scale, rowBasicY + (towerRowH - cbSize) * 0.5f), hasTower("Basic"));
    renderRowTierButtons("Basic", rowBasicY);

    renderCheckbox(glm::vec2(contentLeft + 4.0f * scale, rowMercuryY + (towerRowH - cbSize) * 0.5f), hasTower("Mercury"));
    renderRowTierButtons("Mercury", rowMercuryY);

    renderCheckbox(glm::vec2(contentLeft + 4.0f * scale, rowPistonY + (towerRowH - cbSize) * 0.5f), hasTower("Piston"));
    renderRowTierButtons("Piston", rowPistonY);

    float rowPocketsY = rowPistonY + towerRowH + rowSpacing + 4.0f * scale;
    float cbSizePockets = 16.0f * scale;
    renderCheckbox(glm::vec2(contentLeft + 4.0f * scale, rowPocketsY + (towerRowH - cbSizePockets) * 0.5f), m_hasSidePockets);

    // Секция 4: Нижние кнопки [ Сохранить ] и [ Отмена ]
    float btnH = 32.0f * scale;
    float btnY = modalPos.y + modalH - btnH - 12.0f * scale;
    float saveBtnW = 150.0f * scale;
    float cancelBtnW = 120.0f * scale;
    float btnGap = 20.0f * scale;
    float totalBtnW = saveBtnW + cancelBtnW + btnGap;
    glm::vec2 btnSavePos(modalPos.x + (modalW - totalBtnW) * 0.5f, btnY);
    glm::vec2 btnSaveSize(saveBtnW, btnH);
    glm::vec2 btnCancelPos(btnSavePos.x + saveBtnW + btnGap, btnY);
    glm::vec2 btnCancelSize(cancelBtnW, btnH);

    bool saveHover = isPointInRect(mousePos, btnSavePos, btnSaveSize);
    renderer->drawSprite(whiteTexture, btnSavePos, btnSaveSize, 0.0f, saveHover ? glm::vec3(0.35f, 0.95f, 0.45f) : glm::vec3(0.25f, 0.75f, 0.35f));
    renderer->drawSprite(whiteTexture, btnSavePos + glm::vec2(1.5f), btnSaveSize - glm::vec2(3.0f), 0.0f, saveHover ? glm::vec3(0.18f, 0.48f, 0.24f) : glm::vec3(0.14f, 0.38f, 0.20f));

    bool cancelHover = isPointInRect(mousePos, btnCancelPos, btnCancelSize);
    renderer->drawSprite(whiteTexture, btnCancelPos, btnCancelSize, 0.0f, cancelHover ? glm::vec3(0.70f, 0.75f, 0.85f) : glm::vec3(0.40f, 0.45f, 0.55f));
    renderer->drawSprite(whiteTexture, btnCancelPos + glm::vec2(1.5f), btnCancelSize - glm::vec2(3.0f), 0.0f, cancelHover ? glm::vec3(0.24f, 0.28f, 0.36f) : glm::vec3(0.18f, 0.22f, 0.28f));

    // Секция 3: Контейнер экономики
    float econAreaTop = rowPocketsY + towerRowH + 8.0f * scale;
    float econAreaBottom = btnY - 10.0f * scale;
    float econAreaH = econAreaBottom - econAreaTop;
    glm::vec2 econAreaPos(contentLeft, econAreaTop);
    glm::vec2 econAreaSize(contentW, econAreaH);

    float totalEconH = calculateTotalEconHeight(scale);
    float maxScroll = std::max(0.0f, totalEconH - econAreaH);

    renderer->flush();

    // Включаем аппаратурный Scissor OpenGL для безупречного клиппинга содержимого скролла
    int scissorX = static_cast<int>(econAreaPos.x);
    int scissorY = static_cast<int>(sh - (econAreaPos.y + econAreaSize.y));
    int scissorW = static_cast<int>(econAreaSize.x);
    int scissorH = static_cast<int>(econAreaSize.y);

    glEnable(GL_SCISSOR_TEST);
    glScissor(scissorX, scissorY, scissorW, scissorH);

    // Рисуем спрайты внутри области экономики
    float curY = econAreaTop + 24.0f * scale - m_scrollOffset;

    for (const auto& mob : m_mobTypes) {
        const std::string& key = mob.first;
        float headerRowY = curY;

        // Кнопка [+ Добавить волны]
        glm::vec2 addBtnPos(contentLeft + 150.0f * scale, headerRowY);
        glm::vec2 addBtnSize(135.0f * scale, 22.0f * scale);
        bool addHover = isPointInRect(mousePos, addBtnPos, addBtnSize) && isPointInRect(mousePos, econAreaPos, econAreaSize);

        renderer->drawSprite(whiteTexture, addBtnPos, addBtnSize, 0.0f, addHover ? glm::vec3(0.40f, 0.95f, 0.50f) : glm::vec3(0.25f, 0.75f, 0.35f));
        renderer->drawSprite(whiteTexture, addBtnPos + glm::vec2(1.5f), addBtnSize - glm::vec2(3.0f), 0.0f, addHover ? glm::vec3(0.18f, 0.45f, 0.24f) : glm::vec3(0.12f, 0.32f, 0.16f));

        curY += 24.0f * scale + 4.0f * scale;

        // Строки диапазонов
        auto it = m_mobRules.find(key);
        if (it != m_mobRules.end()) {
            float ruleRowH = 24.0f * scale;
            float boxH = 22.0f * scale;
            size_t ruleCount = it->second.size();

            for (size_t rIdx = 0; rIdx < ruleCount; ++rIdx) {
                float rowY = curY;
                float boxY = rowY + (ruleRowH - boxH) * 0.5f;
                float startX = contentLeft + 10.0f * scale;

                // fromBox
                float fromBoxX = startX + 62.0f * scale;
                glm::vec2 fromBoxPos(fromBoxX, boxY);
                glm::vec2 fromBoxSize(38.0f * scale, boxH);
                bool fromFoc = (m_focused.type == FocusedType::RuleFrom && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                renderInputBox(fromBoxPos, fromBoxSize, fromFoc);

                // toBox
                float toBoxX = fromBoxPos.x + fromBoxSize.x + 30.0f * scale;
                glm::vec2 toBoxPos(toBoxX, boxY);
                glm::vec2 toBoxSize(38.0f * scale, boxH);
                bool toFoc = (m_focused.type == FocusedType::RuleTo && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                renderInputBox(toBoxPos, toBoxSize, toFoc);

                // rewBox
                float rewBoxX = toBoxPos.x + toBoxSize.x + 88.0f * scale;
                glm::vec2 rewBoxPos(rewBoxX, boxY);
                glm::vec2 rewBoxSize(40.0f * scale, boxH);
                bool rewFoc = (m_focused.type == FocusedType::RuleReward && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                renderInputBox(rewBoxPos, rewBoxSize, rewFoc);

                // stepBox
                float stepBoxX = rewBoxPos.x + rewBoxSize.x + 90.0f * scale;
                glm::vec2 stepBoxPos(stepBoxX, boxY);
                glm::vec2 stepBoxSize(36.0f * scale, boxH);
                bool stepFoc = (m_focused.type == FocusedType::RuleStep && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                renderInputBox(stepBoxPos, stepBoxSize, stepFoc);

                // Кнопка [X]
                float delBoxX = stepBoxPos.x + stepBoxSize.x + 12.0f * scale;
                glm::vec2 delBoxPos(delBoxX, boxY);
                glm::vec2 delBoxSize(22.0f * scale, boxH);
                bool delHover = isPointInRect(mousePos, delBoxPos, delBoxSize) && isPointInRect(mousePos, econAreaPos, econAreaSize);

                if (ruleCount > 1) {
                    renderer->drawSprite(whiteTexture, delBoxPos, delBoxSize, 0.0f, delHover ? glm::vec3(0.95f, 0.35f, 0.35f) : glm::vec3(0.65f, 0.22f, 0.22f));
                    renderer->drawSprite(whiteTexture, delBoxPos + glm::vec2(1.5f), delBoxSize - glm::vec2(3.0f), 0.0f, delHover ? glm::vec3(0.45f, 0.14f, 0.14f) : glm::vec3(0.25f, 0.08f, 0.08f));
                } else {
                    renderer->drawSprite(whiteTexture, delBoxPos, delBoxSize, 0.0f, delHover ? glm::vec3(0.85f, 0.55f, 0.25f) : glm::vec3(0.35f, 0.35f, 0.40f));
                    renderer->drawSprite(whiteTexture, delBoxPos + glm::vec2(1.5f), delBoxSize - glm::vec2(3.0f), 0.0f, delHover ? glm::vec3(0.35f, 0.20f, 0.10f) : glm::vec3(0.16f, 0.16f, 0.18f));
                }

                curY += ruleRowH + 4.0f * scale;
            }
        }
        curY += 8.0f * scale;
    }

    // Разделитель и секция прогрессии здоровья (HP) врагов по волнам
    renderer->drawSprite(whiteTexture, glm::vec2(contentLeft, curY + 4.0f * scale), glm::vec2(contentW - 10.0f * scale, 1.5f * scale), 0.0f, glm::vec3(0.28f, 0.34f, 0.44f));
    curY += 12.0f * scale + 24.0f * scale;

    for (const auto& mob : m_mobTypes) {
        const std::string& key = mob.first;
        float headerRowY = curY;

        // Кнопка [+ Добавить волны]
        glm::vec2 addBtnPos(contentLeft + 150.0f * scale, headerRowY);
        glm::vec2 addBtnSize(135.0f * scale, 22.0f * scale);
        bool addHover = isPointInRect(mousePos, addBtnPos, addBtnSize) && isPointInRect(mousePos, econAreaPos, econAreaSize);

        glm::vec3 addBorderColor(0.25f, 0.75f, 0.35f);
        glm::vec3 addFillColor(0.12f, 0.32f, 0.16f);
        if (addHover) {
            addBorderColor = glm::vec3(0.40f, 0.95f, 0.50f);
            addFillColor = glm::vec3(0.18f, 0.45f, 0.24f);
        }
        renderer->drawSprite(whiteTexture, addBtnPos, addBtnSize, 0.0f, addBorderColor);
        renderer->drawSprite(whiteTexture, addBtnPos + glm::vec2(1.5f), addBtnSize - glm::vec2(3.0f), 0.0f, addFillColor);

        curY += 24.0f * scale + 4.0f * scale;

        auto it = m_mobHpRules.find(key);
        if (it != m_mobHpRules.end()) {
            float ruleRowH = 24.0f * scale;
            float boxH = 22.0f * scale;
            size_t ruleCount = it->second.size();

            for (size_t rIdx = 0; rIdx < ruleCount; ++rIdx) {
                float rowY = curY;
                float boxY = rowY + (ruleRowH - boxH) * 0.5f;
                float startX = contentLeft + 10.0f * scale;

                // fromBox
                float fromBoxX = startX + 62.0f * scale;
                glm::vec2 fromBoxPos(fromBoxX, boxY);
                glm::vec2 fromBoxSize(38.0f * scale, boxH);
                bool fromFoc = (m_focused.type == FocusedType::HpRuleFrom && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                renderInputBox(fromBoxPos, fromBoxSize, fromFoc);

                // toBox
                float toBoxX = fromBoxPos.x + fromBoxSize.x + 30.0f * scale;
                glm::vec2 toBoxPos(toBoxX, boxY);
                glm::vec2 toBoxSize(38.0f * scale, boxH);
                bool toFoc = (m_focused.type == FocusedType::HpRuleTo && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                renderInputBox(toBoxPos, toBoxSize, toFoc);

                // hpBox
                float hpBoxX = toBoxPos.x + toBoxSize.x + 38.0f * scale;
                glm::vec2 hpBoxPos(hpBoxX, boxY);
                glm::vec2 hpBoxSize(56.0f * scale, boxH);
                bool hpFoc = (m_focused.type == FocusedType::HpRuleValue && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                renderInputBox(hpBoxPos, hpBoxSize, hpFoc);

                // Кнопка [X]
                float delBoxX = hpBoxPos.x + hpBoxSize.x + 12.0f * scale;
                glm::vec2 delBoxPos(delBoxX, boxY);
                glm::vec2 delBoxSize(22.0f * scale, boxH);
                bool delHover = isPointInRect(mousePos, delBoxPos, delBoxSize) && isPointInRect(mousePos, econAreaPos, econAreaSize);

                glm::vec3 delBorderColor(0.65f, 0.22f, 0.22f);
                glm::vec3 delFillColor(0.25f, 0.08f, 0.08f);
                if (delHover) {
                    delBorderColor = glm::vec3(0.95f, 0.35f, 0.35f);
                    delFillColor = glm::vec3(0.45f, 0.14f, 0.14f);
                }
                renderer->drawSprite(whiteTexture, delBoxPos, delBoxSize, 0.0f, delBorderColor);
                renderer->drawSprite(whiteTexture, delBoxPos + glm::vec2(1.5f), delBoxSize - glm::vec2(3.0f), 0.0f, delFillColor);

                curY += ruleRowH + 4.0f * scale;
            }
        }
        curY += 8.0f * scale;
    }

    // Полоса прокрутки (scrollbar)
    if (maxScroll > 0.0f) {
        float trackX = contentLeft + contentW - 5.0f * scale;
        float trackW = 4.0f * scale;
        renderer->drawSprite(whiteTexture, glm::vec2(trackX, econAreaTop), glm::vec2(trackW, econAreaH), 0.0f, glm::vec3(0.14f, 0.17f, 0.22f));

        float thumbH = std::max(24.0f * scale, econAreaH * (econAreaH / totalEconH));
        float thumbY = econAreaTop + (econAreaH - thumbH) * (m_scrollOffset / maxScroll);
        renderer->drawSprite(whiteTexture, glm::vec2(trackX, thumbY), glm::vec2(trackW, thumbH), 0.0f, glm::vec3(0.45f, 0.55f, 0.70f));
    }

    renderer->flush();

    // 6. Текстовый слой
    if (!textRenderer) {
        glDisable(GL_SCISSOR_TEST);
        return;
    }

    bool showCursor = (m_cursorBlinkTimer < 0.5f);
    float labelScale = 0.44f * scale;
    float valScale = 0.48f * scale;

    // Тексты внутри области экономики (под scissor)
    textRenderer->RenderText("Экономика врагов (награда за уничтожение):", contentLeft, econAreaTop + 4.0f * scale - m_scrollOffset, labelScale, glm::vec3(1.0f, 0.86f, 0.35f));

    float textCurY = econAreaTop + 24.0f * scale - m_scrollOffset;

    for (const auto& mob : m_mobTypes) {
        const std::string& key = mob.first;
        float headerRowY = textCurY;

        // Название моба
        textRenderer->RenderText(mob.second, contentLeft + 4.0f * scale, headerRowY + 4.0f * scale, 0.42f * scale, glm::vec3(0.92f, 0.95f, 1.0f));

        // Кнопка [+ Добавить волны]
        glm::vec2 addBtnPos(contentLeft + 150.0f * scale, headerRowY);
        glm::vec2 addBtnSize(135.0f * scale, 22.0f * scale);
        std::string addText = "+ Добавить волны";
        float addTextW = textRenderer->CalculateTextWidth(addText, 0.38f * scale);
        textRenderer->RenderText(addText, addBtnPos.x + (addBtnSize.x - addTextW) * 0.5f, addBtnPos.y + 4.0f * scale, 0.38f * scale, glm::vec3(0.92f, 1.0f, 0.92f));

        textCurY += 24.0f * scale + 4.0f * scale;

        // Список правил моба
        auto it = m_mobRules.find(key);
        if (it != m_mobRules.end()) {
            float ruleRowH = 24.0f * scale;
            float boxH = 22.0f * scale;
            size_t ruleCount = it->second.size();

            for (size_t rIdx = 0; rIdx < ruleCount; ++rIdx) {
                const auto& er = it->second[rIdx];
                float rowY = textCurY;
                float boxY = rowY + (ruleRowH - boxH) * 0.5f;
                float textY = rowY + 4.0f * scale;

                float startX = contentLeft + 10.0f * scale;

                // "Волны с:"
                textRenderer->RenderText("Волны с:", startX, textY, 0.38f * scale, glm::vec3(0.80f, 0.85f, 0.90f));

                // fromBox
                float fromBoxX = startX + 62.0f * scale;
                glm::vec2 fromBoxPos(fromBoxX, boxY);
                glm::vec2 fromBoxSize(38.0f * scale, boxH);
                bool fromFoc = (m_focused.type == FocusedType::RuleFrom && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                std::string fromDisp = er.fromWaveStr + ((fromFoc && showCursor) ? "|" : "");
                float fW = textRenderer->CalculateTextWidth(fromDisp, 0.40f * scale);
                textRenderer->RenderText(fromDisp, fromBoxPos.x + (fromBoxSize.x - fW) * 0.5f, fromBoxPos.y + 3.5f * scale, 0.40f * scale, glm::vec3(0.90f, 0.95f, 1.0f));

                // "по:"
                float toLabelX = fromBoxPos.x + fromBoxSize.x + 8.0f * scale;
                textRenderer->RenderText("по:", toLabelX, textY, 0.38f * scale, glm::vec3(0.80f, 0.85f, 0.90f));

                // toBox
                float toBoxX = toLabelX + 22.0f * scale;
                glm::vec2 toBoxPos(toBoxX, boxY);
                glm::vec2 toBoxSize(38.0f * scale, boxH);
                bool toFoc = (m_focused.type == FocusedType::RuleTo && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                std::string toDisp = er.toWaveStr + ((toFoc && showCursor) ? "|" : "");
                float tW = textRenderer->CalculateTextWidth(toDisp, 0.40f * scale);
                textRenderer->RenderText(toDisp, toBoxPos.x + (toBoxSize.x - tW) * 0.5f, toBoxPos.y + 3.5f * scale, 0.40f * scale, glm::vec3(0.90f, 0.95f, 1.0f));

                // "Награда ($):"
                float rewLabelX = toBoxPos.x + toBoxSize.x + 10.0f * scale;
                textRenderer->RenderText("Награда ($):", rewLabelX, textY, 0.38f * scale, glm::vec3(0.80f, 0.85f, 0.90f));

                // rewBox
                float rewBoxX = rewLabelX + 78.0f * scale;
                glm::vec2 rewBoxPos(rewBoxX, boxY);
                glm::vec2 rewBoxSize(40.0f * scale, boxH);
                bool rewFoc = (m_focused.type == FocusedType::RuleReward && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                std::string rDisp = er.rewardStr + ((rewFoc && showCursor) ? "|" : "");
                float rW = textRenderer->CalculateTextWidth(rDisp, 0.40f * scale);
                textRenderer->RenderText(rDisp, rewBoxPos.x + (rewBoxSize.x - rW) * 0.5f, rewBoxPos.y + 3.5f * scale, 0.40f * scale, glm::vec3(1.0f, 0.92f, 0.40f));

                // "Каждый N-й:"
                float stepLabelX = rewBoxPos.x + rewBoxSize.x + 10.0f * scale;
                textRenderer->RenderText("Каждый N-й:", stepLabelX, textY, 0.38f * scale, glm::vec3(0.80f, 0.85f, 0.90f));

                // stepBox
                float stepBoxX = stepLabelX + 80.0f * scale;
                glm::vec2 stepBoxPos(stepBoxX, boxY);
                glm::vec2 stepBoxSize(36.0f * scale, boxH);
                bool stepFoc = (m_focused.type == FocusedType::RuleStep && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                std::string sDisp = er.rewardStepStr + ((stepFoc && showCursor) ? "|" : "");
                float sW = textRenderer->CalculateTextWidth(sDisp, 0.40f * scale);
                textRenderer->RenderText(sDisp, stepBoxPos.x + (stepBoxSize.x - sW) * 0.5f, stepBoxPos.y + 3.5f * scale, 0.40f * scale, glm::vec3(0.40f, 0.95f, 0.85f));

                // Кнопка [X]
                float delBoxX = stepBoxPos.x + stepBoxSize.x + 12.0f * scale;
                glm::vec2 delBoxPos(delBoxX, boxY);
                glm::vec2 delBoxSize(22.0f * scale, boxH);
                float xW = textRenderer->CalculateTextWidth("X", 0.38f * scale);
                textRenderer->RenderText("X", delBoxPos.x + (delBoxSize.x - xW) * 0.5f, delBoxPos.y + 3.5f * scale, 0.38f * scale, ruleCount > 1 ? glm::vec3(1.0f, 0.85f, 0.85f) : glm::vec3(0.70f, 0.70f, 0.75f));

                textCurY += ruleRowH + 4.0f * scale;
            }
        }
        textCurY += 8.0f * scale;
    }

    // Тексты секции прогрессии здоровья (HP) врагов по волнам
    textCurY += 12.0f * scale;
    textRenderer->RenderText("Здоровье врагов по волнам (HP):", contentLeft, textCurY + 4.0f * scale, labelScale, glm::vec3(1.0f, 0.86f, 0.35f));
    textCurY += 24.0f * scale;

    for (const auto& mob : m_mobTypes) {
        const std::string& key = mob.first;
        float headerRowY = textCurY;

        // Название моба
        textRenderer->RenderText(mob.second, contentLeft + 4.0f * scale, headerRowY + 4.0f * scale, 0.42f * scale, glm::vec3(0.92f, 0.95f, 1.0f));

        // Кнопка [+ Добавить волны]
        glm::vec2 addBtnPos(contentLeft + 150.0f * scale, headerRowY);
        glm::vec2 addBtnSize(135.0f * scale, 22.0f * scale);
        std::string addText = "+ Добавить волны";
        float addTextW = textRenderer->CalculateTextWidth(addText, 0.38f * scale);
        textRenderer->RenderText(addText, addBtnPos.x + (addBtnSize.x - addTextW) * 0.5f, addBtnPos.y + 4.0f * scale, 0.38f * scale, glm::vec3(0.92f, 1.0f, 0.92f));

        textCurY += 24.0f * scale + 4.0f * scale;

        auto it = m_mobHpRules.find(key);
        if (it != m_mobHpRules.end()) {
            float ruleRowH = 24.0f * scale;
            float boxH = 22.0f * scale;
            size_t ruleCount = it->second.size();

            for (size_t rIdx = 0; rIdx < ruleCount; ++rIdx) {
                const auto& er = it->second[rIdx];
                float rowY = textCurY;
                float boxY = rowY + (ruleRowH - boxH) * 0.5f;
                float textY = rowY + 4.0f * scale;

                float startX = contentLeft + 10.0f * scale;

                // "Волны с:"
                textRenderer->RenderText("Волны с:", startX, textY, 0.38f * scale, glm::vec3(0.80f, 0.85f, 0.90f));

                // fromBox
                float fromBoxX = startX + 62.0f * scale;
                glm::vec2 fromBoxPos(fromBoxX, boxY);
                glm::vec2 fromBoxSize(38.0f * scale, boxH);
                bool fromFoc = (m_focused.type == FocusedType::HpRuleFrom && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                std::string fromDisp = er.fromWaveStr;
                if (fromFoc && showCursor) {
                    fromDisp += "|";
                }
                float fW = textRenderer->CalculateTextWidth(fromDisp, 0.40f * scale);
                textRenderer->RenderText(fromDisp, fromBoxPos.x + (fromBoxSize.x - fW) * 0.5f, fromBoxPos.y + 3.5f * scale, 0.40f * scale, glm::vec3(0.90f, 0.95f, 1.0f));

                // "по:"
                float toLabelX = fromBoxPos.x + fromBoxSize.x + 8.0f * scale;
                textRenderer->RenderText("по:", toLabelX, textY, 0.38f * scale, glm::vec3(0.80f, 0.85f, 0.90f));

                // toBox
                float toBoxX = toLabelX + 22.0f * scale;
                glm::vec2 toBoxPos(toBoxX, boxY);
                glm::vec2 toBoxSize(38.0f * scale, boxH);
                bool toFoc = (m_focused.type == FocusedType::HpRuleTo && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                std::string toDisp = er.toWaveStr;
                if (toFoc && showCursor) {
                    toDisp += "|";
                }
                float tW = textRenderer->CalculateTextWidth(toDisp, 0.40f * scale);
                textRenderer->RenderText(toDisp, toBoxPos.x + (toBoxSize.x - tW) * 0.5f, toBoxPos.y + 3.5f * scale, 0.40f * scale, glm::vec3(0.90f, 0.95f, 1.0f));

                // "HP:"
                float hpLabelX = toBoxPos.x + toBoxSize.x + 10.0f * scale;
                textRenderer->RenderText("HP:", hpLabelX, textY, 0.38f * scale, glm::vec3(0.80f, 0.85f, 0.90f));

                // hpBox
                float hpBoxX = hpLabelX + 28.0f * scale;
                glm::vec2 hpBoxPos(hpBoxX, boxY);
                glm::vec2 hpBoxSize(56.0f * scale, boxH);
                bool hpFoc = (m_focused.type == FocusedType::HpRuleValue && m_focused.mobKey == key && m_focused.ruleIndex == static_cast<int>(rIdx));
                std::string hpDisp = er.hpStr;
                if (hpFoc && showCursor) {
                    hpDisp += "|";
                }
                float hpW = textRenderer->CalculateTextWidth(hpDisp, 0.40f * scale);
                textRenderer->RenderText(hpDisp, hpBoxPos.x + (hpBoxSize.x - hpW) * 0.5f, hpBoxPos.y + 3.5f * scale, 0.40f * scale, glm::vec3(0.55f, 0.98f, 0.60f));

                // Кнопка [X]
                float delBoxX = hpBoxPos.x + hpBoxSize.x + 12.0f * scale;
                glm::vec2 delBoxPos(delBoxX, boxY);
                glm::vec2 delBoxSize(22.0f * scale, boxH);
                float xW = textRenderer->CalculateTextWidth("X", 0.38f * scale);
                textRenderer->RenderText("X", delBoxPos.x + (delBoxSize.x - xW) * 0.5f, delBoxPos.y + 3.5f * scale, 0.38f * scale, glm::vec3(1.0f, 0.85f, 0.85f));

                textCurY += ruleRowH + 4.0f * scale;
            }
        }
        textCurY += 8.0f * scale;
    }

    glDisable(GL_SCISSOR_TEST);

    // 7. Тексты вне скролла (Шапка, Ресурсы, Башни, Кнопки)
    // Шапка
    std::string title = "НАСТРОЙКИ УРОВНЯ И ЭКОНОМИКИ";
    float titleScale = 0.55f * scale;
    float titleW = textRenderer->CalculateTextWidth(title, titleScale);
    textRenderer->RenderText(title, modalPos.x + (modalW - titleW) * 0.5f, modalPos.y + 11.0f * scale, titleScale, glm::vec3(0.95f, 0.96f, 0.98f));

    // Крестик
    float crossTextW = textRenderer->CalculateTextWidth("X", 0.55f * scale);
    textRenderer->RenderText("X", crossPos.x + (crossSize.x - crossTextW) * 0.5f, crossPos.y + 5.0f * scale, 0.55f * scale, crossHover ? glm::vec3(1.0f, 0.90f, 0.90f) : glm::vec3(0.75f, 0.80f, 0.90f));

    // Ресурсы
    textRenderer->RenderText("Стартовые деньги ($):", contentLeft, resRowY, labelScale, glm::vec3(0.85f, 0.90f, 0.98f));
    std::string mDisp = m_moneyStr + ((m_focused.type == FocusedType::Money && showCursor) ? "|" : "");
    textRenderer->RenderText(mDisp, moneyBoxPos.x + 8.0f * scale, moneyBoxPos.y + 5.0f * scale, valScale, glm::vec3(0.98f, 0.98f, 1.0f));

    textRenderer->RenderText("Здоровье базы (HP):", contentLeft + resColW + resGap, resRowY, labelScale, glm::vec3(0.85f, 0.90f, 0.98f));
    std::string hDisp = m_healthStr + ((m_focused.type == FocusedType::Health && showCursor) ? "|" : "");
    textRenderer->RenderText(hDisp, healthBoxPos.x + 8.0f * scale, healthBoxPos.y + 5.0f * scale, valScale, glm::vec3(0.98f, 0.98f, 1.0f));

    // Башни
    textRenderer->RenderText("Разрешенные башни и макс. тир прокачки:", contentLeft, towersLabelY, labelScale, glm::vec3(0.85f, 0.90f, 0.98f));

    auto renderRowLabels = [&](const std::string& name, float rowY) {
        float textY = rowY + 4.0f * scale;
        textRenderer->RenderText(name, contentLeft + 26.0f * scale, textY, 0.44f * scale, glm::vec3(0.92f, 0.95f, 1.0f));
    };

    renderRowLabels("Базовая пушка (Basic)", rowBasicY);
    renderRowLabels("Ртутная вышка (Mercury)", rowMercuryY);
    renderRowLabels("Поршень-толкатель (Piston)", rowPistonY);
    textRenderer->RenderText("Свободные границы (постройка за пределами матрицы)", contentLeft + 26.0f * scale, rowPocketsY + 4.0f * scale, 0.40f * scale, m_hasSidePockets ? glm::vec3(0.4f, 0.9f, 0.5f) : glm::vec3(0.85f, 0.90f, 0.98f));

    auto renderRowTierTexts = [&](const std::string& type, float rowY) {
        float startX = contentLeft + contentW - totalTierGroupW;
        float textY = rowY + 4.0f * scale;
        for (int t = 1; t <= 3; ++t) {
            std::string tLabel = "T" + std::to_string(t);
            float btnX = startX + static_cast<float>(t - 1) * (tierBtnW + tierBtnGap);
            float tW = textRenderer->CalculateTextWidth(tLabel, 0.40f * scale);
            textRenderer->RenderText(tLabel, btnX + (tierBtnW - tW) * 0.5f, textY, 0.40f * scale, glm::vec3(0.95f, 0.95f, 0.95f));
        }
    };

    renderRowTierTexts("Basic", rowBasicY);
    renderRowTierTexts("Mercury", rowMercuryY);
    renderRowTierTexts("Piston", rowPistonY);

    // Нижние кнопки
    float saveTextW = textRenderer->CalculateTextWidth("Сохранить", labelScale);
    textRenderer->RenderText("Сохранить", btnSavePos.x + (btnSaveSize.x - saveTextW) * 0.5f, btnSavePos.y + 8.0f * scale, labelScale, glm::vec3(1.0f, 1.0f, 1.0f));

    float cancelTextW = textRenderer->CalculateTextWidth("Отмена", labelScale);
    textRenderer->RenderText("Отмена", btnCancelPos.x + (btnCancelSize.x - cancelTextW) * 0.5f, btnCancelPos.y + 8.0f * scale, labelScale, glm::vec3(0.95f, 0.95f, 0.95f));
}
