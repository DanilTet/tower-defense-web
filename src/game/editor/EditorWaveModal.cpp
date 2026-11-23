#include "EditorWaveModal.h"
#include "EditorToolbarUI.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include "../ui/UICommon.h"
#include "../core/InputManager.h"
#include <GLFW/glfw3.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

static float s_cachedLabelCountW = 0.0f;
static float s_cachedLabelIntervalW = 0.0f;
static float s_cachedLabelPauseW = 0.0f;

struct EnemyTypeChoice {
    std::string id;
    std::string label;
    glm::vec3 color;
    glm::vec3 baseBg;
    glm::vec3 border;
};

// Полный реестр для корректного отображения уже созданных пачек на существующих картах
static const std::vector<EnemyTypeChoice>& getAllEnemyTypeInfo() {
    static const std::vector<EnemyTypeChoice> all = {
        { "Coal",           "Уголёк",           glm::vec3(0.85f, 0.60f, 0.35f), glm::vec3(0.22f, 0.20f, 0.18f), glm::vec3(0.60f, 0.40f, 0.25f) },
        { "Spark",          "Искра",            glm::vec3(1.00f, 0.80f, 0.25f), glm::vec3(0.28f, 0.16f, 0.08f), glm::vec3(0.92f, 0.55f, 0.12f) },
        { "Basic",          "Basic",            glm::vec3(0.40f, 0.90f, 1.00f), glm::vec3(0.15f, 0.28f, 0.45f), glm::vec3(0.30f, 0.60f, 0.95f) },
        { "Fast",           "Fast",             glm::vec3(1.00f, 0.90f, 0.30f), glm::vec3(0.38f, 0.32f, 0.12f), glm::vec3(0.95f, 0.80f, 0.20f) },
        { "Tank",           "Tank",             glm::vec3(1.00f, 0.45f, 0.45f), glm::vec3(0.42f, 0.16f, 0.16f), glm::vec3(0.95f, 0.32f, 0.32f) },
        { "Mercury_Large",  "Ртуть (Большая)",  glm::vec3(0.85f, 0.90f, 0.98f), glm::vec3(0.24f, 0.28f, 0.36f), glm::vec3(0.70f, 0.78f, 0.90f) },
        { "Mercury_Medium", "Ртуть (Средняя)",  glm::vec3(0.75f, 0.82f, 0.92f), glm::vec3(0.20f, 0.24f, 0.32f), glm::vec3(0.60f, 0.70f, 0.82f) },
        { "Mercury_Small",  "Ртуть (Малая)",    glm::vec3(0.65f, 0.75f, 0.85f), glm::vec3(0.18f, 0.22f, 0.28f), glm::vec3(0.50f, 0.60f, 0.75f) }
    };
    return all;
}

// Список доступных для выбора типов врагов в редакторе (Coal, Spark, Mercury)
static const std::vector<EnemyTypeChoice>& getEnemyTypeChoices() {
    static const std::vector<EnemyTypeChoice> choices = {
        { "Coal",           "Уголёк",           glm::vec3(0.85f, 0.60f, 0.35f), glm::vec3(0.22f, 0.20f, 0.18f), glm::vec3(0.60f, 0.40f, 0.25f) },
        { "Spark",          "Искра",            glm::vec3(1.00f, 0.80f, 0.25f), glm::vec3(0.28f, 0.16f, 0.08f), glm::vec3(0.92f, 0.55f, 0.12f) },
        // Временно скрыты устаревшие прототипы врагов:
        // { "Basic",          "Basic",            glm::vec3(0.40f, 0.90f, 1.00f), glm::vec3(0.15f, 0.28f, 0.45f), glm::vec3(0.30f, 0.60f, 0.95f) },
        // { "Fast",           "Fast",             glm::vec3(1.00f, 0.90f, 0.30f), glm::vec3(0.38f, 0.32f, 0.12f), glm::vec3(0.95f, 0.80f, 0.20f) },
        // { "Tank",           "Tank",             glm::vec3(1.00f, 0.45f, 0.45f), glm::vec3(0.42f, 0.16f, 0.16f), glm::vec3(0.95f, 0.32f, 0.32f) },
        { "Mercury_Large",  "Ртуть (Большая)",  glm::vec3(0.85f, 0.90f, 0.98f), glm::vec3(0.24f, 0.28f, 0.36f), glm::vec3(0.70f, 0.78f, 0.90f) },
        { "Mercury_Medium", "Ртуть (Средняя)",  glm::vec3(0.75f, 0.82f, 0.92f), glm::vec3(0.20f, 0.24f, 0.32f), glm::vec3(0.60f, 0.70f, 0.82f) },
        { "Mercury_Small",  "Ртуть (Малая)",    glm::vec3(0.65f, 0.75f, 0.85f), glm::vec3(0.18f, 0.22f, 0.28f), glm::vec3(0.50f, 0.60f, 0.75f) }
    };
    return choices;
}

struct Row2Layout {
    glm::vec2 labelCountPos{0.0f};
    glm::vec2 cntDecPos{0.0f};
    glm::vec2 cntBoxPos{0.0f};
    glm::vec2 cntBoxSize{0.0f};
    glm::vec2 cntIncPos{0.0f};

    glm::vec2 labelIntervalPos{0.0f};
    glm::vec2 rateDecPos{0.0f};
    glm::vec2 rateBoxPos{0.0f};
    glm::vec2 rateBoxSize{0.0f};
    glm::vec2 rateIncPos{0.0f};

    glm::vec2 labelPausePos{0.0f};
    glm::vec2 pauseDecPos{0.0f};
    glm::vec2 pauseBoxPos{0.0f};
    glm::vec2 pauseBoxSize{0.0f};
    glm::vec2 pauseIncPos{0.0f};

    glm::vec2 nudgeSize{0.0f};
};

static Row2Layout computeRow2Layout(glm::vec2 cardPos, float cardW, float scale, TextRenderer* textRenderer) {
    float fSub = std::clamp(0.52f * scale, 0.42f, 0.62f);
    float wLabelCount = (textRenderer ? textRenderer->CalculateTextWidth("Кол-во:", fSub)
                                      : (s_cachedLabelCountW > 0.0f ? s_cachedLabelCountW : 112.0f * fSub));
    float wLabelInterval = (textRenderer ? textRenderer->CalculateTextWidth("Интервал:", fSub)
                                         : (s_cachedLabelIntervalW > 0.0f ? s_cachedLabelIntervalW : 144.0f * fSub));
    float wLabelPause = (textRenderer ? textRenderer->CalculateTextWidth("Пауза:", fSub)
                                      : (s_cachedLabelPauseW > 0.0f ? s_cachedLabelPauseW : 96.0f * fSub));

    if (textRenderer) {
        s_cachedLabelCountW = wLabelCount;
        s_cachedLabelIntervalW = wLabelInterval;
        s_cachedLabelPauseW = wLabelPause;
    }

    float row2Y = cardPos.y + 46.0f * scale;
    float btnH = std::clamp(26.0f * scale, 22.0f, 30.0f);
    float nudgeW = std::clamp(22.0f * scale, 18.0f, 26.0f);
    float countBoxW = std::clamp(46.0f * scale, 38.0f, 54.0f);
    float intervalBoxW = std::clamp(52.0f * scale, 42.0f, 62.0f);
    float pauseBoxW = std::clamp(52.0f * scale, 42.0f, 62.0f);

    float labelToBtnGap = 6.0f * scale;
    float btnToBoxGap = 2.0f * scale;

    float ctrlCountW = nudgeW * 2.0f + countBoxW + btnToBoxGap * 2.0f;
    float ctrlIntervalW = nudgeW * 2.0f + intervalBoxW + btnToBoxGap * 2.0f;
    float ctrlPauseW = nudgeW * 2.0f + pauseBoxW + btnToBoxGap * 2.0f;

    float block1W = wLabelCount + labelToBtnGap + ctrlCountW;
    float block2W = wLabelInterval + labelToBtnGap + ctrlIntervalW;
    float block3W = wLabelPause + labelToBtnGap + ctrlPauseW;

    float cardPadLeft = 14.0f * scale;
    float cardPadRight = 14.0f * scale;
    float totalAvail = cardW - cardPadLeft - cardPadRight;
    float totalBlocksW = block1W + block2W + block3W;
    float remaining = totalAvail - totalBlocksW;
    float gap = std::max(16.0f * scale, remaining * 0.5f);

    float b1X = cardPos.x + cardPadLeft;
    float b2X = b1X + block1W + gap;
    float b3X = b2X + block2W + gap;

    Row2Layout r;
    r.nudgeSize = glm::vec2(nudgeW, btnH);

    // Block 1: Count
    r.labelCountPos = glm::vec2(b1X, row2Y + (btnH - fSub * 28.0f) * 0.5f + 2.0f);
    r.cntDecPos = glm::vec2(b1X + wLabelCount + labelToBtnGap, row2Y);
    r.cntBoxPos = glm::vec2(r.cntDecPos.x + nudgeW + btnToBoxGap, row2Y);
    r.cntBoxSize = glm::vec2(countBoxW, btnH);
    r.cntIncPos = glm::vec2(r.cntBoxPos.x + countBoxW + btnToBoxGap, row2Y);

    // Block 2: Interval
    r.labelIntervalPos = glm::vec2(b2X, row2Y + (btnH - fSub * 28.0f) * 0.5f + 2.0f);
    r.rateDecPos = glm::vec2(b2X + wLabelInterval + labelToBtnGap, row2Y);
    r.rateBoxPos = glm::vec2(r.rateDecPos.x + nudgeW + btnToBoxGap, row2Y);
    r.rateBoxSize = glm::vec2(intervalBoxW, btnH);
    r.rateIncPos = glm::vec2(r.rateBoxPos.x + intervalBoxW + btnToBoxGap, row2Y);

    // Block 3: Pause
    r.labelPausePos = glm::vec2(b3X, row2Y + (btnH - fSub * 28.0f) * 0.5f + 2.0f);
    r.pauseDecPos = glm::vec2(b3X + wLabelPause + labelToBtnGap, row2Y);
    r.pauseBoxPos = glm::vec2(r.pauseDecPos.x + nudgeW + btnToBoxGap, row2Y);
    r.pauseBoxSize = glm::vec2(pauseBoxW, btnH);
    r.pauseIncPos = glm::vec2(r.pauseBoxPos.x + pauseBoxW + btnToBoxGap, row2Y);

    return r;
}

bool EditorWaveModal::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= (rectPos.x + rectSize.x) &&
           point.y >= rectPos.y && point.y <= (rectPos.y + rectSize.y);
}

std::string EditorWaveModal::formatFloat1(float val) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << val;
    return ss.str();
}

std::string EditorWaveModal::getSpawnerLabel(int spawnerId) {
    if (spawnerId < 0) return "Спавнер: Все";
    return "Спавнер #" + std::to_string(spawnerId);
}

glm::vec3 EditorWaveModal::getSpawnerColor(int spawnerId) {
    if (spawnerId < 0) return EditorToolbarUI::getIdColor(-1); // Золотистый/дефолтный цвет ("Все")
    return EditorToolbarUI::getIdColor(spawnerId); // 0 = Циан (#0), 1 = Оранжевый (#1), 2 = Лаймовый (#2), 3 = Пурпурный (#3)
}

void EditorWaveModal::open() {
    m_isOpen = true;
    m_openDropdownPartIdx = -1;
}

void EditorWaveModal::open(std::vector<WaveConfig>& waves) {
    m_isOpen = true;
    m_openDropdownPartIdx = -1;
    if (m_selectedWaveIdx < 0 || m_selectedWaveIdx >= static_cast<int>(waves.size())) {
        m_selectedWaveIdx = 0;
    }
    m_wavePartsScrollOffset = 0;
    m_wavesScrollOffset = 0;
}

void EditorWaveModal::open(LevelMapData& mapData) {
    m_boundMapData = &mapData;
    open(mapData.waves);
}

void EditorWaveModal::close() {
    m_isOpen = false;
    m_openDropdownPartIdx = -1;
    m_focusedField = WaveFocusedField::None;
    m_focusedPartIdx = -1;
    m_inputText.clear();
    m_fieldJustFocused = false;
}

void EditorWaveModal::toggle() {
    if (m_isOpen) close();
    else open();
}

void EditorWaveModal::toggle(std::vector<WaveConfig>& waves) {
    if (m_isOpen) close();
    else open(waves);
}

void EditorWaveModal::toggle(LevelMapData& mapData) {
    if (m_isOpen) close();
    else open(mapData);
}

void EditorWaveModal::update(float dt) {
    if (!m_isOpen) return;
    m_cursorBlinkTimer += dt;
    if (m_cursorBlinkTimer >= 1.06f) {
        m_cursorBlinkTimer = 0.0f;
    }
}

void EditorWaveModal::commitFocusedInput(std::vector<WaveConfig>& waves) {
    if (m_focusedField == WaveFocusedField::None || m_focusedPartIdx < 0) {
        m_focusedField = WaveFocusedField::None;
        m_focusedPartIdx = -1;
        m_inputText.clear();
        m_fieldJustFocused = false;
        return;
    }
    if (m_selectedWaveIdx < 0 || m_selectedWaveIdx >= static_cast<int>(waves.size())) {
        m_focusedField = WaveFocusedField::None;
        m_focusedPartIdx = -1;
        m_inputText.clear();
        m_fieldJustFocused = false;
        return;
    }

    WaveConfig& wave = waves[m_selectedWaveIdx];
    if (m_focusedPartIdx >= static_cast<int>(wave.parts.size())) {
        m_focusedField = WaveFocusedField::None;
        m_focusedPartIdx = -1;
        m_inputText.clear();
        m_fieldJustFocused = false;
        return;
    }

    WavePart& part = wave.parts[m_focusedPartIdx];

    if (!m_inputText.empty()) {
        try {
            if (m_focusedField == WaveFocusedField::Count) {
                int val = std::stoi(m_inputText);
                part.count = std::clamp(val, 1, 999);
            } else if (m_focusedField == WaveFocusedField::Interval) {
                float val = std::stof(m_inputText);
                part.spawnInterwal = std::clamp(val, 0.05f, 30.0f);
            } else if (m_focusedField == WaveFocusedField::Delay) {
                float val = std::stof(m_inputText);
                part.delayAfter = std::clamp(val, 0.0f, 120.0f);
            }
        } catch (...) {
            // Оставляем прошлое корректное значение
        }
    }

    m_focusedField = WaveFocusedField::None;
    m_focusedPartIdx = -1;
    m_inputText.clear();
    m_fieldJustFocused = false;
}

void EditorWaveModal::startEditingField(int partIdx, WaveFocusedField field, const std::vector<WaveConfig>& waves) {
    if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
        m_openDropdownPartIdx = -1;
        m_focusedPartIdx = partIdx;
        m_focusedField = field;
        m_fieldJustFocused = true;
        m_cursorBlinkTimer = 0.0f;

        const WaveConfig& wave = waves[m_selectedWaveIdx];
        if (partIdx >= 0 && partIdx < static_cast<int>(wave.parts.size())) {
            const WavePart& part = wave.parts[partIdx];
            if (field == WaveFocusedField::Count) {
                m_inputText = std::to_string(part.count);
            } else if (field == WaveFocusedField::Interval) {
                m_inputText = formatFloat1(part.spawnInterwal);
            } else if (field == WaveFocusedField::Delay) {
                m_inputText = formatFloat1(part.delayAfter);
            }
        }
    }
}

void EditorWaveModal::cycleNextInputField(std::vector<WaveConfig>& waves) {
    if (m_focusedField == WaveFocusedField::None || m_focusedPartIdx < 0) return;
    int curPart = m_focusedPartIdx;
    WaveFocusedField curField = m_focusedField;
    commitFocusedInput(waves);

    if (m_selectedWaveIdx < 0 || m_selectedWaveIdx >= static_cast<int>(waves.size())) return;
    WaveConfig& wave = waves[m_selectedWaveIdx];
    if (wave.parts.empty()) return;

    if (curField == WaveFocusedField::Count) {
        startEditingField(curPart, WaveFocusedField::Interval, waves);
    } else if (curField == WaveFocusedField::Interval) {
        startEditingField(curPart, WaveFocusedField::Delay, waves);
    } else if (curField == WaveFocusedField::Delay) {
        int nextPart = (curPart + 1) % static_cast<int>(wave.parts.size());
        startEditingField(nextPart, WaveFocusedField::Count, waves);
    }
}

void EditorWaveModal::updateLayout(int screenWidth, int screenHeight) {
    float scale = GetUIScale(screenWidth, screenHeight);
    m_layout.scale = scale;

    float mW = std::clamp(1020.0f * scale, 860.0f, std::min(1150.0f, static_cast<float>(screenWidth) - 40.0f));
    float mH = std::clamp(640.0f * scale, 520.0f, std::min(800.0f, static_cast<float>(screenHeight) - 40.0f));
    float mX = (static_cast<float>(screenWidth) - mW) * 0.5f;
    float mY = (static_cast<float>(screenHeight) - mH) * 0.5f;

    m_layout.modalPos = glm::vec2(mX, mY);
    m_layout.modalSize = glm::vec2(mW, mH);

    m_layout.headerH = std::clamp(40.0f * scale, 34.0f, 48.0f);
    m_layout.footerH = std::clamp(32.0f * scale, 26.0f, 38.0f);

    m_layout.btnCloseSize = glm::vec2(std::clamp(68.0f * scale, 56.0f, 80.0f), std::clamp(26.0f * scale, 22.0f, 30.0f));
    m_layout.btnClosePos = glm::vec2(mX + mW - m_layout.btnCloseSize.x - 10.0f * scale, mY + (m_layout.headerH - m_layout.btnCloseSize.y) * 0.5f);

    float pad = std::clamp(10.0f * scale, 8.0f, 14.0f);
    float colY = mY + m_layout.headerH + pad;
    float colH = mH - m_layout.headerH - m_layout.footerH - pad * 2.0f;

    // Левая колонка (Список волн)
    float colLeftW = std::clamp(236.0f * scale, 195.0f, 260.0f);
    m_layout.colLeftPos = glm::vec2(mX + pad, colY);
    m_layout.colLeftSize = glm::vec2(colLeftW, colH);

    float lBtnPad = std::clamp(6.0f * scale, 4.0f, 8.0f);
    float lBtnGap = std::clamp(6.0f * scale, 4.0f, 8.0f);
    float lBtnW = (colLeftW - lBtnPad * 2.0f - lBtnGap) * 0.5f;
    float lBtnH = std::clamp(28.0f * scale, 24.0f, 32.0f);

    m_layout.btnAddWavePos = glm::vec2(m_layout.colLeftPos.x + lBtnPad, m_layout.colLeftPos.y + lBtnPad);
    m_layout.btnAddWaveSize = glm::vec2(lBtnW, lBtnH);

    m_layout.btnDuplicateWavePos = glm::vec2(m_layout.btnAddWavePos.x + lBtnW + lBtnGap, m_layout.btnAddWavePos.y);
    m_layout.btnDuplicateWaveSize = glm::vec2(lBtnW, lBtnH);

    m_layout.btnWaveScrollSize = glm::vec2(std::clamp(22.0f * scale, 18.0f, 26.0f), std::clamp(22.0f * scale, 18.0f, 26.0f));
    m_layout.waveListStartY = m_layout.btnAddWavePos.y + lBtnH + std::clamp(8.0f * scale, 6.0f, 10.0f);
    m_layout.waveItemH = std::clamp(36.0f * scale, 30.0f, 42.0f);
    m_layout.waveItemGap = std::clamp(4.0f * scale, 3.0f, 6.0f);

    float waveListAvailH = (m_layout.colLeftPos.y + colH - lBtnPad) - m_layout.waveListStartY;
    m_layout.maxVisibleWaves = std::max(1, static_cast<int>(waveListAvailH / (m_layout.waveItemH + m_layout.waveItemGap)));

    // Правая колонка: растягиваем ближе к правому краю модального окна (отступ справа ~20.0f * scale)
    float rightEdge = mX + mW - 20.0f * scale;
    float colRightX = m_layout.colLeftPos.x + colLeftW + pad;
    float colRightW = rightEdge - colRightX;

    // Спецификация Y-координат:
    // 1) Заголовок и кнопка [+ Добавить пачку]: headerY = modalY + 50.0f * scale.
    float headerY = mY + 50.0f * scale;
    m_layout.rHeaderH = std::clamp(32.0f * scale, 28.0f, 36.0f);
    m_layout.colRightPos = glm::vec2(colRightX, headerY);
    m_layout.colRightSize = glm::vec2(colRightW, (mY + mH - m_layout.footerH - 12.0f * scale) - headerY);

    m_layout.btnAddPartSize = glm::vec2(std::clamp(145.0f * scale, 120.0f, 165.0f), std::clamp(28.0f * scale, 24.0f, 32.0f));
    m_layout.btnAddPartPos = glm::vec2(rightEdge - m_layout.btnAddPartSize.x, headerY);

    // 2) Карточки пачек стартуют строго под шапкой: cardsStartY = headerY + 45.0f * scale.
    m_layout.cardsStartY = headerY + 45.0f * scale;

    // 3) Каждая следующая карточка смещается вниз на (cardH + 12.0f * scale).
    float row2BtnH = std::clamp(26.0f * scale, 22.0f, 30.0f);
    float minCardH = (46.0f * scale + row2BtnH) + 8.0f * scale;
    m_layout.cardH = std::max(std::clamp(92.0f * scale, 84.0f, 130.0f), minCardH);
    m_layout.cardGap = 12.0f * scale;

    // 4) Карточки и шапка строго внутри рамки модального окна
    float modalBottomY = mY + mH - m_layout.footerH - 12.0f * scale;
    float cardsAvailH = std::max(0.0f, modalBottomY - m_layout.cardsStartY);
    m_layout.maxVisibleParts = std::max(1, static_cast<int>(cardsAvailH / (m_layout.cardH + m_layout.cardGap)));
}

bool EditorWaveModal::handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt,
                                  int screenWidth, int screenHeight, float, float,
                                  std::vector<WaveConfig>& waves, bool& isDirty) {
    if (!m_isOpen) return false;

    updateLayout(screenWidth, screenHeight);
    const WaveModalLayout& l = m_layout;
    float scale = l.scale;

    bool isShiftDown = window && (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                                 glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    // 1. Ввод с клавиатуры в активное числовое поле
    if (window && m_focusedField != WaveFocusedField::None && m_focusedPartIdx >= 0) {
        auto checkKeyInput = [&](int key) -> bool {
            bool down = (glfwGetKey(window, key) == GLFW_PRESS);
            auto& ks = m_keyStates[key];
            if (down) {
                if (!ks.isDown) {
                    ks.isDown = true;
                    ks.holdTimer = 0.0f;
                    ks.repeatTimer = 0.0f;
                    return true;
                } else {
                    ks.holdTimer += dt;
                    if (ks.holdTimer >= 0.38f) {
                        ks.repeatTimer += dt;
                        if (ks.repeatTimer >= 0.05f) {
                            ks.repeatTimer = 0.0f;
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
        };

        if (checkKeyInput(GLFW_KEY_BACKSPACE)) {
            if (m_fieldJustFocused) {
                m_inputText.clear();
                m_fieldJustFocused = false;
            } else if (!m_inputText.empty()) {
                m_inputText.pop_back();
            }
            m_cursorBlinkTimer = 0.0f;
            isDirty = true;
        }
        else if (checkKeyInput(GLFW_KEY_ENTER) || checkKeyInput(GLFW_KEY_KP_ENTER)) {
            commitFocusedInput(waves);
            isDirty = true;
        }
        else if (checkKeyInput(GLFW_KEY_TAB)) {
            cycleNextInputField(waves);
            isDirty = true;
        }
        else if (checkKeyInput(GLFW_KEY_ESCAPE)) {
            commitFocusedInput(waves);
        }

        for (int k = 0; k <= 9; ++k) {
            if (checkKeyInput(GLFW_KEY_0 + k) || checkKeyInput(GLFW_KEY_KP_0 + k)) {
                char ch = '0' + k;
                if (m_fieldJustFocused) {
                    m_inputText.clear();
                    m_fieldJustFocused = false;
                }
                if (m_focusedField == WaveFocusedField::Count) {
                    if (m_inputText.length() < 3) {
                        m_inputText += ch;
                        m_cursorBlinkTimer = 0.0f;
                    }
                } else {
                    if (m_inputText.length() < 5) {
                        m_inputText += ch;
                        m_cursorBlinkTimer = 0.0f;
                    }
                }
                isDirty = true;
            }
        }

        if (m_focusedField != WaveFocusedField::Count) {
            if (checkKeyInput(GLFW_KEY_PERIOD) || checkKeyInput(GLFW_KEY_COMMA) || checkKeyInput(GLFW_KEY_KP_DECIMAL)) {
                if (m_fieldJustFocused) {
                    m_inputText.clear();
                    m_fieldJustFocused = false;
                }
                if (m_inputText.find('.') == std::string::npos && m_inputText.length() < 5) {
                    if (m_inputText.empty()) m_inputText += "0.";
                    else m_inputText += '.';
                    m_cursorBlinkTimer = 0.0f;
                }
                isDirty = true;
            }
        }
    }

    // 2. Обработка скролла колесиком мыши (пачки и список волн)
    float scrollY = InputManager::getScrollY();
    if (std::abs(scrollY) > 0.01f) {
        // Скролл списка пачек в правой колонке
        if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
            WaveConfig& curWave = waves[m_selectedWaveIdx];
            float cardsAreaH = static_cast<float>(l.maxVisibleParts) * (l.cardH + l.cardGap);
            glm::vec2 cardsAreaPos(l.colRightPos.x, l.cardsStartY);
            glm::vec2 cardsAreaSize(l.colRightSize.x, cardsAreaH);
            if (isPointInRect(mousePos, cardsAreaPos, cardsAreaSize)) {
                int maxOffset = std::max(0, static_cast<int>(curWave.parts.size()) - l.maxVisibleParts);
                if (scrollY > 0.0f) {
                    m_wavePartsScrollOffset = std::max(0, m_wavePartsScrollOffset - 1);
                } else {
                    m_wavePartsScrollOffset = std::min(maxOffset, m_wavePartsScrollOffset + 1);
                }
                InputManager::clearScroll();
                return true;
            }
        }

        // Скролл списка волн в левой колонке
        if (isPointInRect(mousePos, l.colLeftPos, l.colLeftSize)) {
            int maxOffset = std::max(0, static_cast<int>(waves.size()) - l.maxVisibleWaves);
            if (scrollY > 0.0f) {
                m_wavesScrollOffset = std::max(0, m_wavesScrollOffset - 1);
            } else {
                m_wavesScrollOffset = std::min(maxOffset, m_wavesScrollOffset + 1);
            }
            InputManager::clearScroll();
            return true;
        }
    }

    // 3. Обработка кликов мыши
    bool isClick = leftDown && !m_wasLeftDown;
    m_wasLeftDown = leftDown;

    if (isClick) {
        // Проверяем выпадающий список типов врагов (если открыт)
        if (m_openDropdownPartIdx >= 0 && m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
            WaveConfig& curWave = waves[m_selectedWaveIdx];
            int visIdx = m_openDropdownPartIdx - m_wavePartsScrollOffset;
            if (visIdx >= 0 && visIdx < l.maxVisibleParts && m_openDropdownPartIdx < static_cast<int>(curWave.parts.size())) {
                glm::vec2 cardPos(l.colRightPos.x, l.cardsStartY + static_cast<float>(visIdx) * (l.cardH + l.cardGap));
                glm::vec2 typeBtnPos(cardPos.x + 40.0f * scale, cardPos.y + 7.0f * scale);
                glm::vec2 typeBtnSize(130.0f * scale, 26.0f * scale);

                float dropX = typeBtnPos.x;
                float dropW = 145.0f * scale;
                float itemH = std::clamp(28.0f * scale, 22.0f, 32.0f);
                const auto& choices = getEnemyTypeChoices();
                float totalDropH = itemH * static_cast<float>(choices.size());
                float dropY = typeBtnPos.y + typeBtnSize.y + 2.0f;
                if (dropY + totalDropH > l.modalPos.y + l.modalSize.y - l.footerH) {
                    dropY = typeBtnPos.y - totalDropH - 2.0f;
                }

                for (size_t c = 0; c < choices.size(); ++c) {
                    if (isPointInRect(mousePos, glm::vec2(dropX, dropY + itemH * static_cast<float>(c)), glm::vec2(dropW, itemH))) {
                        curWave.parts[m_openDropdownPartIdx].type = choices[c].id;
                        m_openDropdownPartIdx = -1;
                        isDirty = true;
                        return true;
                    }
                }
                if (isPointInRect(mousePos, typeBtnPos, typeBtnSize)) {
                    m_openDropdownPartIdx = -1;
                    return true;
                }
            }
            m_openDropdownPartIdx = -1;
            return true;
        }

        // Если кликнули мимо центрированного модального окна — закрываем модалку и поглощаем клик
        if (!isPointInRect(mousePos, l.modalPos, l.modalSize)) {
            commitFocusedInput(waves);
            close();
            return true;
        }

        // Кнопка [X Close] в шапке
        if (isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize)) {
            commitFocusedInput(waves);
            close();
            return true;
        }

        // Кнопка [+ Волна]
        if (isPointInRect(mousePos, l.btnAddWavePos, l.btnAddWaveSize)) {
            commitFocusedInput(waves);
            WaveConfig newWave;
            WavePart p;
            p.type = "Coal";
            p.count = 10;
            p.spawnInterwal = 0.8f;
            p.delayAfter = 2.0f;
            p.spawnerId = -1;
            newWave.parts.push_back(p);
            waves.push_back(newWave);
            m_selectedWaveIdx = static_cast<int>(waves.size()) - 1;
            m_wavePartsScrollOffset = 0;
            if (m_selectedWaveIdx >= m_wavesScrollOffset + l.maxVisibleWaves) {
                m_wavesScrollOffset = m_selectedWaveIdx - l.maxVisibleWaves + 1;
            }
            isDirty = true;
            return true;
        }

        // Кнопка [Дублировать]
        if (isPointInRect(mousePos, l.btnDuplicateWavePos, l.btnDuplicateWaveSize)) {
            commitFocusedInput(waves);
            if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
                WaveConfig duplicated = waves[m_selectedWaveIdx];
                waves.insert(waves.begin() + m_selectedWaveIdx + 1, duplicated);
                m_selectedWaveIdx++;
                m_wavePartsScrollOffset = 0;
                if (m_selectedWaveIdx >= m_wavesScrollOffset + l.maxVisibleWaves) {
                    m_wavesScrollOffset = m_selectedWaveIdx - l.maxVisibleWaves + 1;
                }
                isDirty = true;
                return true;
            }
        }

        // Список волн в левой колонке
        float itemW = l.colLeftSize.x - std::clamp(12.0f * scale, 8.0f, 16.0f);
        float moveBtnW = std::clamp(20.0f * scale, 16.0f, 24.0f);
        float delBtnW = std::clamp(22.0f * scale, 18.0f, 26.0f);
        float btnGap = 2.0f * scale;
        int maxWaves = std::min(l.maxVisibleWaves, static_cast<int>(waves.size()) - m_wavesScrollOffset);

        for (int v = 0; v < maxWaves; ++v) {
            int wIdx = m_wavesScrollOffset + v;
            if (wIdx < 0 || wIdx >= static_cast<int>(waves.size())) break;

            float itemY = l.waveListStartY + static_cast<float>(v) * (l.waveItemH + l.waveItemGap);
            glm::vec2 itemPos(l.colLeftPos.x + std::clamp(6.0f * scale, 4.0f, 8.0f), itemY);
            glm::vec2 itemSize(itemW - delBtnW - moveBtnW * 2.0f - btnGap * 3.0f, l.waveItemH);

            glm::vec2 upPos(itemPos.x + itemSize.x + btnGap, itemY);
            glm::vec2 upSize(moveBtnW, l.waveItemH);

            glm::vec2 downPos(upPos.x + moveBtnW + btnGap, itemY);
            glm::vec2 downSize(moveBtnW, l.waveItemH);

            glm::vec2 delPos(downPos.x + moveBtnW + btnGap, itemY);
            glm::vec2 delSize(delBtnW, l.waveItemH);

            if (isPointInRect(mousePos, itemPos, itemSize)) {
                commitFocusedInput(waves);
                m_selectedWaveIdx = wIdx;
                m_wavePartsScrollOffset = 0;
                return true;
            }

            // Перемещение волны вверх [^]
            if (isPointInRect(mousePos, upPos, upSize)) {
                if (wIdx > 0) {
                    commitFocusedInput(waves);
                    std::swap(waves[wIdx], waves[wIdx - 1]);
                    if (m_selectedWaveIdx == wIdx) {
                        m_selectedWaveIdx = wIdx - 1;
                    } else if (m_selectedWaveIdx == wIdx - 1) {
                        m_selectedWaveIdx = wIdx;
                    }
                    isDirty = true;
                    return true;
                }
            }

            // Перемещение волны вниз [v]
            if (isPointInRect(mousePos, downPos, downSize)) {
                if (wIdx + 1 < static_cast<int>(waves.size())) {
                    commitFocusedInput(waves);
                    std::swap(waves[wIdx], waves[wIdx + 1]);
                    if (m_selectedWaveIdx == wIdx) {
                        m_selectedWaveIdx = wIdx + 1;
                    } else if (m_selectedWaveIdx == wIdx + 1) {
                        m_selectedWaveIdx = wIdx;
                    }
                    isDirty = true;
                    return true;
                }
            }

            if (isPointInRect(mousePos, delPos, delSize)) {
                commitFocusedInput(waves);
                waves.erase(waves.begin() + wIdx);
                if (waves.empty()) {
                    WaveConfig defW;
                    defW.parts.push_back({ "Coal", 10, 0.8f, 2.0f, -1 });
                    waves.push_back(defW);
                }
                m_selectedWaveIdx = std::clamp(m_selectedWaveIdx, 0, static_cast<int>(waves.size()) - 1);
                m_wavePartsScrollOffset = 0;
                int maxOffset = std::max(0, static_cast<int>(waves.size()) - l.maxVisibleWaves);
                m_wavesScrollOffset = std::clamp(m_wavesScrollOffset, 0, maxOffset);
                isDirty = true;
                return true;
            }
        }

        // Правая колонка: пачки волны
        if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
            WaveConfig& curWave = waves[m_selectedWaveIdx];

            // [+ Add Batch]
            if (isPointInRect(mousePos, l.btnAddPartPos, l.btnAddPartSize)) {
                commitFocusedInput(waves);
                WavePart p;
                p.type = "Coal";
                p.count = 10;
                p.spawnInterwal = 0.8f;
                p.delayAfter = 2.0f;
                p.spawnerId = -1;
                curWave.parts.push_back(p);
                if (static_cast<int>(curWave.parts.size()) > l.maxVisibleParts) {
                    m_wavePartsScrollOffset = static_cast<int>(curWave.parts.size()) - l.maxVisibleParts;
                }
                isDirty = true;
                return true;
            }

            int maxOffset = std::max(0, static_cast<int>(curWave.parts.size()) - l.maxVisibleParts);
            m_wavePartsScrollOffset = std::clamp(m_wavePartsScrollOffset, 0, maxOffset);

            for (int v = 0; v < l.maxVisibleParts; ++v) {
                int pIdx = m_wavePartsScrollOffset + v;
                if (pIdx >= static_cast<int>(curWave.parts.size())) break;

                WavePart& part = curWave.parts[pIdx];
                glm::vec2 cardPos(l.colRightPos.x, l.cardsStartY + static_cast<float>(v) * (l.cardH + l.cardGap));
                glm::vec2 cardSize(l.colRightSize.x, l.cardH);

                // Ряд 1: Кнопка выбора типа моба
                glm::vec2 typeBtnPos(cardPos.x + 40.0f * scale, cardPos.y + 7.0f * scale);
                glm::vec2 typeBtnSize(130.0f * scale, 26.0f * scale);
                if (isPointInRect(mousePos, typeBtnPos, typeBtnSize)) {
                    commitFocusedInput(waves);
                    m_openDropdownPartIdx = (m_openDropdownPartIdx == pIdx) ? -1 : pIdx;
                    return true;
                }

                // Ряд 1: Кнопка Spawner ID ("Все" (-1) -> S#0 (0) -> S#1 (1) -> S#2 (2) -> S#3 (3) -> "Все" (-1))
                glm::vec2 spawnerBtnPos(typeBtnPos.x + typeBtnSize.x + 8.0f * scale, cardPos.y + 7.0f * scale);
                glm::vec2 spawnerBtnSize(130.0f * scale, 26.0f * scale);
                if (isPointInRect(mousePos, spawnerBtnPos, spawnerBtnSize)) {
                    commitFocusedInput(waves);
                    if (part.spawnerId < 0) {
                        part.spawnerId = 0;
                    } else if (part.spawnerId >= 3) {
                        part.spawnerId = -1;
                    } else {
                        part.spawnerId++;
                    }
                    isDirty = true;
                    return true;
                }

                float moveBtnW = std::clamp(22.0f * scale, 18.0f, 26.0f);
                float delBtnW = std::clamp(24.0f * scale, 20.0f, 26.0f);
                float btnGap = 3.0f * scale;

                glm::vec2 delPartBtnPos(cardPos.x + cardSize.x - delBtnW - 4.0f * scale, cardPos.y + 7.0f * scale);
                glm::vec2 delPartBtnSize(delBtnW, 26.0f * scale);

                glm::vec2 downPartBtnPos(delPartBtnPos.x - moveBtnW - btnGap, cardPos.y + 7.0f * scale);
                glm::vec2 downPartBtnSize(moveBtnW, 26.0f * scale);

                glm::vec2 upPartBtnPos(downPartBtnPos.x - moveBtnW - btnGap, cardPos.y + 7.0f * scale);
                glm::vec2 upPartBtnSize(moveBtnW, 26.0f * scale);

                // Перемещение пачки вверх [^]
                if (isPointInRect(mousePos, upPartBtnPos, upPartBtnSize)) {
                    if (pIdx > 0) {
                        commitFocusedInput(waves);
                        std::swap(curWave.parts[pIdx], curWave.parts[pIdx - 1]);
                        if (m_focusedPartIdx == pIdx) m_focusedPartIdx = pIdx - 1;
                        else if (m_focusedPartIdx == pIdx - 1) m_focusedPartIdx = pIdx;
                        if (m_openDropdownPartIdx == pIdx) m_openDropdownPartIdx = pIdx - 1;
                        else if (m_openDropdownPartIdx == pIdx - 1) m_openDropdownPartIdx = pIdx;
                        isDirty = true;
                        return true;
                    }
                }

                // Перемещение пачки вниз [v]
                if (isPointInRect(mousePos, downPartBtnPos, downPartBtnSize)) {
                    if (pIdx + 1 < static_cast<int>(curWave.parts.size())) {
                        commitFocusedInput(waves);
                        std::swap(curWave.parts[pIdx], curWave.parts[pIdx + 1]);
                        if (m_focusedPartIdx == pIdx) m_focusedPartIdx = pIdx + 1;
                        else if (m_focusedPartIdx == pIdx + 1) m_focusedPartIdx = pIdx;
                        if (m_openDropdownPartIdx == pIdx) m_openDropdownPartIdx = pIdx + 1;
                        else if (m_openDropdownPartIdx == pIdx + 1) m_openDropdownPartIdx = pIdx;
                        isDirty = true;
                        return true;
                    }
                }

                // Ряд 1: Удаление пачки [X]
                if (isPointInRect(mousePos, delPartBtnPos, delPartBtnSize)) {
                    commitFocusedInput(waves);
                    curWave.parts.erase(curWave.parts.begin() + pIdx);
                    int newMaxOffset = std::max(0, static_cast<int>(curWave.parts.size()) - l.maxVisibleParts);
                    m_wavePartsScrollOffset = std::clamp(m_wavePartsScrollOffset, 0, newMaxOffset);
                    isDirty = true;
                    return true;
                }

                // Ряд 2: Контролы Count, Rate, Pause (адаптивная сетка)
                Row2Layout r2 = computeRow2Layout(cardPos, cardSize.x, scale, nullptr);

                // Count
                if (isPointInRect(mousePos, r2.cntDecPos, r2.nudgeSize)) {
                    commitFocusedInput(waves);
                    int delta = isShiftDown ? 5 : 1;
                    part.count = std::max(1, part.count - delta);
                    isDirty = true;
                    return true;
                }
                if (isPointInRect(mousePos, r2.cntBoxPos, r2.cntBoxSize)) {
                    commitFocusedInput(waves);
                    startEditingField(pIdx, WaveFocusedField::Count, waves);
                    return true;
                }
                if (isPointInRect(mousePos, r2.cntIncPos, r2.nudgeSize)) {
                    commitFocusedInput(waves);
                    int delta = isShiftDown ? 5 : 1;
                    part.count = std::min(999, part.count + delta);
                    isDirty = true;
                    return true;
                }

                // Rate
                if (isPointInRect(mousePos, r2.rateDecPos, r2.nudgeSize)) {
                    commitFocusedInput(waves);
                    part.spawnInterwal = std::max(0.05f, std::round((part.spawnInterwal - 0.1f) * 10.0f) / 10.0f);
                    isDirty = true;
                    return true;
                }
                if (isPointInRect(mousePos, r2.rateBoxPos, r2.rateBoxSize)) {
                    commitFocusedInput(waves);
                    startEditingField(pIdx, WaveFocusedField::Interval, waves);
                    return true;
                }
                if (isPointInRect(mousePos, r2.rateIncPos, r2.nudgeSize)) {
                    commitFocusedInput(waves);
                    part.spawnInterwal = std::min(30.0f, std::round((part.spawnInterwal + 0.1f) * 10.0f) / 10.0f);
                    isDirty = true;
                    return true;
                }

                // Pause
                if (isPointInRect(mousePos, r2.pauseDecPos, r2.nudgeSize)) {
                    commitFocusedInput(waves);
                    part.delayAfter = std::max(0.0f, std::round((part.delayAfter - 0.5f) * 10.0f) / 10.0f);
                    isDirty = true;
                    return true;
                }
                if (isPointInRect(mousePos, r2.pauseBoxPos, r2.pauseBoxSize)) {
                    commitFocusedInput(waves);
                    startEditingField(pIdx, WaveFocusedField::Delay, waves);
                    return true;
                }
                if (isPointInRect(mousePos, r2.pauseIncPos, r2.nudgeSize)) {
                    commitFocusedInput(waves);
                    part.delayAfter = std::min(120.0f, std::round((part.delayAfter + 0.5f) * 10.0f) / 10.0f);
                    isDirty = true;
                    return true;
                }
            }
        }

        commitFocusedInput(waves);
        return true;
    }

    return true;
}

bool EditorWaveModal::handleInput(float mouseX, float mouseY, bool mousePressed, LevelMapData& mapData, int, int) {
    bool dummyDirty = false;
    return handleInput(nullptr, glm::vec2(mouseX, mouseY), mousePressed, 0.016f,
                       1920, 1080, 48.0f, 64.0f, mapData.waves, dummyDirty);
}

void EditorWaveModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                             const Texture2D* whiteTexture,
                             int screenWidth, int screenHeight, float, float,
                             glm::vec2 mousePos, const std::vector<WaveConfig>& waves) {
    if (!m_isOpen || !renderer || !whiteTexture) return;

    updateLayout(screenWidth, screenHeight);
    const WaveModalLayout& l = m_layout;
    float scale = l.scale;

    // 1. ПОЛУПРОЗРАЧНЫЙ БЭКДРОП
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(0.0f), glm::vec2(screenWidth, screenHeight), 0.0f,
                             glm::vec4(0.02f, 0.03f, 0.05f, 0.75f));

    // 2. ЦЕНТРИРОВАННОЕ ОКНО РЕДАКТОРА
    renderer->drawSprite(whiteTexture, l.modalPos - glm::vec2(2.0f), l.modalSize + glm::vec2(4.0f), 0.0f, glm::vec3(0.25f, 0.60f, 0.90f));
    renderer->drawSprite(whiteTexture, l.modalPos, l.modalSize, 0.0f, glm::vec3(0.09f, 0.10f, 0.14f));

    // ХЕДЕР ОКНА
    renderer->drawSprite(whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, l.headerH), 0.0f, glm::vec3(0.13f, 0.15f, 0.20f));
    renderer->drawSprite(whiteTexture, glm::vec2(l.modalPos.x, l.modalPos.y + l.headerH - 1.0f), glm::vec2(l.modalSize.x, 1.0f), 0.0f, glm::vec3(0.25f, 0.60f, 0.90f));

    // Кнопка [X Close] в хедере
    bool closeHov = isPointInRect(mousePos, l.btnClosePos, l.btnCloseSize);
    renderer->drawSprite(whiteTexture, l.btnClosePos, l.btnCloseSize, 0.0f, closeHov ? glm::vec3(0.85f, 0.30f, 0.30f) : glm::vec3(0.55f, 0.20f, 0.20f));
    renderer->drawSprite(whiteTexture, l.btnClosePos + glm::vec2(1.0f), l.btnCloseSize - glm::vec2(2.0f), 0.0f, closeHov ? glm::vec3(0.48f, 0.16f, 0.16f) : glm::vec3(0.32f, 0.12f, 0.12f));

    // ФУТЕР ОКНА
    float footerY = l.modalPos.y + l.modalSize.y - l.footerH;
    renderer->drawSprite(whiteTexture, glm::vec2(l.modalPos.x, footerY), glm::vec2(l.modalSize.x, l.footerH), 0.0f, glm::vec3(0.11f, 0.12f, 0.16f));
    renderer->drawSprite(whiteTexture, glm::vec2(l.modalPos.x, footerY), glm::vec2(l.modalSize.x, 1.0f), 0.0f, glm::vec3(0.22f, 0.25f, 0.32f));

    // 3. ЛЕВАЯ КОЛОНКА (Список волн)
    renderer->drawSprite(whiteTexture, l.colLeftPos, l.colLeftSize, 0.0f, glm::vec3(0.20f, 0.23f, 0.29f));
    renderer->drawSprite(whiteTexture, l.colLeftPos + glm::vec2(1.0f), l.colLeftSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.07f, 0.08f, 0.11f));

    // Кнопка [+ Волна]
    bool addWaveHov = isPointInRect(mousePos, l.btnAddWavePos, l.btnAddWaveSize);
    renderer->drawSprite(whiteTexture, l.btnAddWavePos, l.btnAddWaveSize, 0.0f, addWaveHov ? glm::vec3(0.35f, 0.85f, 0.50f) : glm::vec3(0.25f, 0.70f, 0.40f));
    renderer->drawSprite(whiteTexture, l.btnAddWavePos + glm::vec2(1.0f), l.btnAddWaveSize - glm::vec2(2.0f), 0.0f, addWaveHov ? glm::vec3(0.18f, 0.42f, 0.24f) : glm::vec3(0.13f, 0.32f, 0.18f));

    // Кнопка [Дублировать]
    bool dupWaveHov = isPointInRect(mousePos, l.btnDuplicateWavePos, l.btnDuplicateWaveSize);
    renderer->drawSprite(whiteTexture, l.btnDuplicateWavePos, l.btnDuplicateWaveSize, 0.0f, dupWaveHov ? glm::vec3(0.35f, 0.85f, 1.0f) : glm::vec3(0.25f, 0.65f, 0.90f));
    renderer->drawSprite(whiteTexture, l.btnDuplicateWavePos + glm::vec2(1.0f), l.btnDuplicateWaveSize - glm::vec2(2.0f), 0.0f, dupWaveHov ? glm::vec3(0.16f, 0.36f, 0.54f) : glm::vec3(0.12f, 0.26f, 0.40f));

    // Карточки списка волн
    float itemW = l.colLeftSize.x - std::clamp(12.0f * scale, 8.0f, 16.0f);
    float moveBtnW = std::clamp(20.0f * scale, 16.0f, 24.0f);
    float delBtnW = std::clamp(22.0f * scale, 18.0f, 26.0f);
    float btnGap = 2.0f * scale;
    int maxWaves = std::min(l.maxVisibleWaves, static_cast<int>(waves.size()) - m_wavesScrollOffset);

    for (int v = 0; v < maxWaves; ++v) {
        int wIdx = m_wavesScrollOffset + v;
        if (wIdx < 0 || wIdx >= static_cast<int>(waves.size())) break;

        float itemY = l.waveListStartY + static_cast<float>(v) * (l.waveItemH + l.waveItemGap);
        glm::vec2 itemPos(l.colLeftPos.x + std::clamp(6.0f * scale, 4.0f, 8.0f), itemY);
        glm::vec2 itemSize(itemW - delBtnW - moveBtnW * 2.0f - btnGap * 3.0f, l.waveItemH);

        glm::vec2 upPos(itemPos.x + itemSize.x + btnGap, itemY);
        glm::vec2 upSize(moveBtnW, l.waveItemH);

        glm::vec2 downPos(upPos.x + moveBtnW + btnGap, itemY);
        glm::vec2 downSize(moveBtnW, l.waveItemH);

        glm::vec2 delPos(downPos.x + moveBtnW + btnGap, itemY);
        glm::vec2 delSize(delBtnW, l.waveItemH);

        bool isSel = (wIdx == m_selectedWaveIdx);
        bool waveHov = isPointInRect(mousePos, itemPos, itemSize);
        bool canMoveUp = (wIdx > 0);
        bool canMoveDown = (wIdx + 1 < static_cast<int>(waves.size()));
        bool upHov = canMoveUp && isPointInRect(mousePos, upPos, upSize);
        bool downHov = canMoveDown && isPointInRect(mousePos, downPos, downSize);
        bool delHov = isPointInRect(mousePos, delPos, delSize);

        glm::vec3 waveBorder = isSel ? glm::vec3(0.35f, 0.85f, 1.0f) : (waveHov ? glm::vec3(0.35f, 0.42f, 0.55f) : glm::vec3(0.24f, 0.28f, 0.36f));
        glm::vec3 waveBg = isSel ? glm::vec3(0.18f, 0.34f, 0.54f) : (waveHov ? glm::vec3(0.15f, 0.18f, 0.24f) : glm::vec3(0.11f, 0.13f, 0.17f));

        renderer->drawSprite(whiteTexture, itemPos, itemSize, 0.0f, waveBorder);
        renderer->drawSprite(whiteTexture, itemPos + glm::vec2(1.0f), itemSize - glm::vec2(2.0f), 0.0f, waveBg);

        // Кнопка [^]
        glm::vec3 upBorder = canMoveUp ? (upHov ? glm::vec3(0.50f, 0.55f, 0.65f) : glm::vec3(0.28f, 0.32f, 0.40f)) : glm::vec3(0.18f, 0.20f, 0.24f);
        glm::vec3 upBg = canMoveUp ? (upHov ? glm::vec3(0.22f, 0.25f, 0.32f) : glm::vec3(0.14f, 0.16f, 0.20f)) : glm::vec3(0.10f, 0.11f, 0.13f);
        renderer->drawSprite(whiteTexture, upPos, upSize, 0.0f, upBorder);
        renderer->drawSprite(whiteTexture, upPos + glm::vec2(1.0f), upSize - glm::vec2(2.0f), 0.0f, upBg);

        // Кнопка [v]
        glm::vec3 downBorder = canMoveDown ? (downHov ? glm::vec3(0.50f, 0.55f, 0.65f) : glm::vec3(0.28f, 0.32f, 0.40f)) : glm::vec3(0.18f, 0.20f, 0.24f);
        glm::vec3 downBg = canMoveDown ? (downHov ? glm::vec3(0.22f, 0.25f, 0.32f) : glm::vec3(0.14f, 0.16f, 0.20f)) : glm::vec3(0.10f, 0.11f, 0.13f);
        renderer->drawSprite(whiteTexture, downPos, downSize, 0.0f, downBorder);
        renderer->drawSprite(whiteTexture, downPos + glm::vec2(1.0f), downSize - glm::vec2(2.0f), 0.0f, downBg);

        // Кнопка [X]
        renderer->drawSprite(whiteTexture, delPos, delSize, 0.0f, delHov ? glm::vec3(0.80f, 0.25f, 0.25f) : glm::vec3(0.50f, 0.20f, 0.20f));
        renderer->drawSprite(whiteTexture, delPos + glm::vec2(1.0f), delSize - glm::vec2(2.0f), 0.0f, delHov ? glm::vec3(0.40f, 0.15f, 0.15f) : glm::vec3(0.25f, 0.11f, 0.11f));
    }

    // 4. ПРАВАЯ КОЛОНКА (Конфигуратор волны)
    // Кнопка [+ Add Batch]
    bool addPartHov = isPointInRect(mousePos, l.btnAddPartPos, l.btnAddPartSize);
    renderer->drawSprite(whiteTexture, l.btnAddPartPos, l.btnAddPartSize, 0.0f, addPartHov ? glm::vec3(0.35f, 0.85f, 0.50f) : glm::vec3(0.25f, 0.70f, 0.40f));
    renderer->drawSprite(whiteTexture, l.btnAddPartPos + glm::vec2(1.0f), l.btnAddPartSize - glm::vec2(2.0f), 0.0f, addPartHov ? glm::vec3(0.18f, 0.42f, 0.24f) : glm::vec3(0.13f, 0.32f, 0.18f));

    // Расчет метрик волны (Длительность и общее число мобов)
    int totalMobsInWave = 0;
    float waveTotalTime = 0.0f;
    if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
        const auto& curWave = waves[m_selectedWaveIdx];
        for (const auto& p : curWave.parts) {
            totalMobsInWave += p.count;
            float partTime = (p.count > 0 ? (p.count - 1) * p.spawnInterwal : 0.0f) + p.delayAfter;
            waveTotalTime += partTime;
        }

        // Карточки пачек
        if (curWave.parts.empty()) {
            glm::vec2 emptyPos(l.colRightPos.x, l.cardsStartY);
            glm::vec2 emptySize(l.colRightSize.x, 90.0f * scale);
            renderer->drawSprite(whiteTexture, emptyPos, emptySize, 0.0f, glm::vec3(0.22f, 0.25f, 0.30f));
            renderer->drawSprite(whiteTexture, emptyPos + glm::vec2(1.0f), emptySize - glm::vec2(2.0f), 0.0f, glm::vec3(0.10f, 0.11f, 0.14f));
        } else {
            for (int v = 0; v < l.maxVisibleParts; ++v) {
                int pIdx = m_wavePartsScrollOffset + v;
                if (pIdx >= static_cast<int>(curWave.parts.size())) break;
                const auto& part = curWave.parts[pIdx];

                glm::vec2 cardPos(l.colRightPos.x, l.cardsStartY + static_cast<float>(v) * (l.cardH + l.cardGap));
                glm::vec2 cardSize(l.colRightSize.x, l.cardH);

                renderer->drawSprite(whiteTexture, cardPos, cardSize, 0.0f, glm::vec3(0.24f, 0.28f, 0.36f));
                renderer->drawSprite(whiteTexture, cardPos + glm::vec2(1.0f), cardSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.12f, 0.14f, 0.18f));

                // Кнопка выбора типа моба
                glm::vec2 typeBtnPos(cardPos.x + 40.0f * scale, cardPos.y + 7.0f * scale);
                glm::vec2 typeBtnSize(130.0f * scale, 26.0f * scale);
                bool typeHov = isPointInRect(mousePos, typeBtnPos, typeBtnSize);
                bool isDropOpen = (m_openDropdownPartIdx == pIdx);

                glm::vec3 typeBg(0.15f, 0.28f, 0.45f);
                glm::vec3 typeBorder(0.30f, 0.60f, 0.95f);
                for (const auto& ch : getAllEnemyTypeInfo()) {
                    if (part.type == ch.id) {
                        typeBorder = isDropOpen ? glm::min(ch.border * 1.15f, glm::vec3(1.0f)) : ch.border;
                        typeBg = isDropOpen ? (ch.baseBg * 1.35f) : (typeHov ? (ch.baseBg * 1.20f) : ch.baseBg);
                        break;
                    }
                }
                renderer->drawSprite(whiteTexture, typeBtnPos, typeBtnSize, 0.0f, typeBorder);
                renderer->drawSprite(whiteTexture, typeBtnPos + glm::vec2(1.0f), typeBtnSize - glm::vec2(2.0f), 0.0f, typeBg);

                // Кнопка Spawner ID (Все / S#1 / S#2 / S#3)
                glm::vec2 spawnerBtnPos(typeBtnPos.x + typeBtnSize.x + 8.0f * scale, cardPos.y + 7.0f * scale);
                glm::vec2 spawnerBtnSize(130.0f * scale, 26.0f * scale);
                bool spHov = isPointInRect(mousePos, spawnerBtnPos, spawnerBtnSize);
                glm::vec3 spColor = getSpawnerColor(part.spawnerId);
                glm::vec3 spBg = spColor * (spHov ? 0.32f : 0.20f);

                renderer->drawSprite(whiteTexture, spawnerBtnPos, spawnerBtnSize, 0.0f, spHov ? glm::min(spColor * 1.2f, glm::vec3(1.0f)) : spColor);
                renderer->drawSprite(whiteTexture, spawnerBtnPos + glm::vec2(1.0f), spawnerBtnSize - glm::vec2(2.0f), 0.0f, spBg);

                float moveBtnW = std::clamp(22.0f * scale, 18.0f, 26.0f);
                float delBtnW = std::clamp(24.0f * scale, 20.0f, 26.0f);
                float btnGap = 3.0f * scale;

                glm::vec2 delPartBtnPos(cardPos.x + cardSize.x - delBtnW - 4.0f * scale, cardPos.y + 7.0f * scale);
                glm::vec2 delPartBtnSize(delBtnW, 26.0f * scale);

                glm::vec2 downPartBtnPos(delPartBtnPos.x - moveBtnW - btnGap, cardPos.y + 7.0f * scale);
                glm::vec2 downPartBtnSize(moveBtnW, 26.0f * scale);

                glm::vec2 upPartBtnPos(downPartBtnPos.x - moveBtnW - btnGap, cardPos.y + 7.0f * scale);
                glm::vec2 upPartBtnSize(moveBtnW, 26.0f * scale);

                bool canPartUp = (pIdx > 0);
                bool canPartDown = (pIdx + 1 < static_cast<int>(curWave.parts.size()));
                bool upPartHov = canPartUp && isPointInRect(mousePos, upPartBtnPos, upPartBtnSize);
                bool downPartHov = canPartDown && isPointInRect(mousePos, downPartBtnPos, downPartBtnSize);

                // Кнопка пачки [^]
                glm::vec3 upPBorder = canPartUp ? (upPartHov ? glm::vec3(0.50f, 0.55f, 0.65f) : glm::vec3(0.28f, 0.32f, 0.40f)) : glm::vec3(0.18f, 0.20f, 0.24f);
                glm::vec3 upPBg = canPartUp ? (upPartHov ? glm::vec3(0.22f, 0.25f, 0.32f) : glm::vec3(0.14f, 0.16f, 0.20f)) : glm::vec3(0.10f, 0.11f, 0.13f);
                renderer->drawSprite(whiteTexture, upPartBtnPos, upPartBtnSize, 0.0f, upPBorder);
                renderer->drawSprite(whiteTexture, upPartBtnPos + glm::vec2(1.0f), upPartBtnSize - glm::vec2(2.0f), 0.0f, upPBg);

                // Кнопка пачки [v]
                glm::vec3 downPBorder = canPartDown ? (downPartHov ? glm::vec3(0.50f, 0.55f, 0.65f) : glm::vec3(0.28f, 0.32f, 0.40f)) : glm::vec3(0.18f, 0.20f, 0.24f);
                glm::vec3 downPBg = canPartDown ? (downPartHov ? glm::vec3(0.22f, 0.25f, 0.32f) : glm::vec3(0.14f, 0.16f, 0.20f)) : glm::vec3(0.10f, 0.11f, 0.13f);
                renderer->drawSprite(whiteTexture, downPartBtnPos, downPartBtnSize, 0.0f, downPBorder);
                renderer->drawSprite(whiteTexture, downPartBtnPos + glm::vec2(1.0f), downPartBtnSize - glm::vec2(2.0f), 0.0f, downPBg);

                // Кнопка удаления пачки [X]
                bool delPartHov = isPointInRect(mousePos, delPartBtnPos, delPartBtnSize);
                renderer->drawSprite(whiteTexture, delPartBtnPos, delPartBtnSize, 0.0f, delPartHov ? glm::vec3(0.80f, 0.25f, 0.25f) : glm::vec3(0.60f, 0.20f, 0.20f));
                renderer->drawSprite(whiteTexture, delPartBtnPos + glm::vec2(1.0f), delPartBtnSize - glm::vec2(2.0f), 0.0f, delPartHov ? glm::vec3(0.45f, 0.16f, 0.16f) : glm::vec3(0.32f, 0.12f, 0.12f));

                // Ряд 2: Контролы параметров (адаптивная верстка)
                Row2Layout r2 = computeRow2Layout(cardPos, cardSize.x, scale, textRenderer);

                // Count
                bool cDecHov = isPointInRect(mousePos, r2.cntDecPos, r2.nudgeSize);
                renderer->drawSprite(whiteTexture, r2.cntDecPos, r2.nudgeSize, 0.0f, cDecHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                renderer->drawSprite(whiteTexture, r2.cntDecPos + glm::vec2(1.0f), r2.nudgeSize - glm::vec2(2.0f), 0.0f, cDecHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                bool isCntFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Count);
                bool cBoxHov = isPointInRect(mousePos, r2.cntBoxPos, r2.cntBoxSize);
                glm::vec3 cntBorder = isCntFocused ? glm::vec3(0.35f, 0.85f, 1.0f) : (cBoxHov ? glm::vec3(0.40f, 0.46f, 0.58f) : glm::vec3(0.28f, 0.32f, 0.40f));
                glm::vec3 cntBoxBg = isCntFocused ? glm::vec3(0.10f, 0.15f, 0.24f) : (cBoxHov ? glm::vec3(0.11f, 0.13f, 0.17f) : glm::vec3(0.08f, 0.09f, 0.12f));
                renderer->drawSprite(whiteTexture, r2.cntBoxPos, r2.cntBoxSize, 0.0f, cntBorder);
                renderer->drawSprite(whiteTexture, r2.cntBoxPos + glm::vec2(1.0f), r2.cntBoxSize - glm::vec2(2.0f), 0.0f, cntBoxBg);

                bool cIncHov = isPointInRect(mousePos, r2.cntIncPos, r2.nudgeSize);
                renderer->drawSprite(whiteTexture, r2.cntIncPos, r2.nudgeSize, 0.0f, cIncHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                renderer->drawSprite(whiteTexture, r2.cntIncPos + glm::vec2(1.0f), r2.nudgeSize - glm::vec2(2.0f), 0.0f, cIncHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                // Rate
                bool rDecHov = isPointInRect(mousePos, r2.rateDecPos, r2.nudgeSize);
                renderer->drawSprite(whiteTexture, r2.rateDecPos, r2.nudgeSize, 0.0f, rDecHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                renderer->drawSprite(whiteTexture, r2.rateDecPos + glm::vec2(1.0f), r2.nudgeSize - glm::vec2(2.0f), 0.0f, rDecHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                bool isRateFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Interval);
                bool rBoxHov = isPointInRect(mousePos, r2.rateBoxPos, r2.rateBoxSize);
                glm::vec3 rateBorder = isRateFocused ? glm::vec3(0.35f, 0.85f, 1.0f) : (rBoxHov ? glm::vec3(0.40f, 0.46f, 0.58f) : glm::vec3(0.28f, 0.32f, 0.40f));
                glm::vec3 rateBoxBg = isRateFocused ? glm::vec3(0.10f, 0.15f, 0.24f) : (rBoxHov ? glm::vec3(0.11f, 0.13f, 0.17f) : glm::vec3(0.08f, 0.09f, 0.12f));
                renderer->drawSprite(whiteTexture, r2.rateBoxPos, r2.rateBoxSize, 0.0f, rateBorder);
                renderer->drawSprite(whiteTexture, r2.rateBoxPos + glm::vec2(1.0f), r2.rateBoxSize - glm::vec2(2.0f), 0.0f, rateBoxBg);

                bool rIncHov = isPointInRect(mousePos, r2.rateIncPos, r2.nudgeSize);
                renderer->drawSprite(whiteTexture, r2.rateIncPos, r2.nudgeSize, 0.0f, rIncHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                renderer->drawSprite(whiteTexture, r2.rateIncPos + glm::vec2(1.0f), r2.nudgeSize - glm::vec2(2.0f), 0.0f, rIncHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                // Pause
                bool pDecHov = isPointInRect(mousePos, r2.pauseDecPos, r2.nudgeSize);
                renderer->drawSprite(whiteTexture, r2.pauseDecPos, r2.nudgeSize, 0.0f, pDecHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                renderer->drawSprite(whiteTexture, r2.pauseDecPos + glm::vec2(1.0f), r2.nudgeSize - glm::vec2(2.0f), 0.0f, pDecHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                bool isPauseFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Delay);
                bool pBoxHov = isPointInRect(mousePos, r2.pauseBoxPos, r2.pauseBoxSize);
                glm::vec3 pauseBorder = isPauseFocused ? glm::vec3(0.35f, 0.85f, 1.0f) : (pBoxHov ? glm::vec3(0.40f, 0.46f, 0.58f) : glm::vec3(0.28f, 0.32f, 0.40f));
                glm::vec3 pauseBoxBg = isPauseFocused ? glm::vec3(0.10f, 0.15f, 0.24f) : (pBoxHov ? glm::vec3(0.11f, 0.13f, 0.17f) : glm::vec3(0.08f, 0.09f, 0.12f));
                renderer->drawSprite(whiteTexture, r2.pauseBoxPos, r2.pauseBoxSize, 0.0f, pauseBorder);
                renderer->drawSprite(whiteTexture, r2.pauseBoxPos + glm::vec2(1.0f), r2.pauseBoxSize - glm::vec2(2.0f), 0.0f, pauseBoxBg);

                bool pIncHov = isPointInRect(mousePos, r2.pauseIncPos, r2.nudgeSize);
                renderer->drawSprite(whiteTexture, r2.pauseIncPos, r2.nudgeSize, 0.0f, pIncHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
            }
        }

        // Вертикальный скроллбар для пачек (если пачек больше, чем умещается)
        if (static_cast<int>(curWave.parts.size()) > l.maxVisibleParts) {
            float trackW = 5.0f * scale;
            float trackX = l.colRightPos.x + l.colRightSize.x - trackW - 4.0f * scale;
            float trackY = l.cardsStartY;
            float trackH = static_cast<float>(l.maxVisibleParts) * (l.cardH + l.cardGap) - l.cardGap;

            // Фон трека скроллбара
            renderer->drawSprite(whiteTexture, glm::vec2(trackX, trackY), glm::vec2(trackW, trackH), 0.0f, glm::vec3(0.12f, 0.14f, 0.18f));

            // Бегунок скроллбара
            int totalParts = static_cast<int>(curWave.parts.size());
            int maxOff = totalParts - l.maxVisibleParts;
            float thumbRatio = static_cast<float>(l.maxVisibleParts) / static_cast<float>(totalParts);
            float thumbH = std::max(18.0f * scale, trackH * thumbRatio);
            float thumbProgress = (maxOff > 0) ? (static_cast<float>(m_wavePartsScrollOffset) / static_cast<float>(maxOff)) : 0.0f;
            float thumbY = trackY + (trackH - thumbH) * thumbProgress;

            renderer->drawSprite(whiteTexture, glm::vec2(trackX, thumbY), glm::vec2(trackW, thumbH), 0.0f, glm::vec3(0.38f, 0.44f, 0.55f));
        }
    }

    renderer->flush();

    // 6. ОТРИСОВКА ТЕКСТА
    if (textRenderer) {
        float fTitle = std::clamp(0.68f * scale, 0.55f, 0.85f);
        float fSub   = std::clamp(0.52f * scale, 0.42f, 0.62f);
        float fBtn   = std::clamp(0.50f * scale, 0.40f, 0.60f);

        // Хедер
        textRenderer->RenderText("РЕДАКТОР ВОЛН", l.modalPos.x + 16.0f * scale, l.modalPos.y + (l.headerH - fTitle * 28.0f) * 0.5f + 2.0f, fTitle, glm::vec3(0.35f, 0.85f, 1.0f));

        float twClose = textRenderer->CalculateTextWidth("[X] Закрыть", fBtn);
        textRenderer->RenderText("[X] Закрыть", l.btnClosePos.x + (l.btnCloseSize.x - twClose) * 0.5f, l.btnClosePos.y + (l.btnCloseSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(1.0f, 0.88f, 0.88f));

        // Кнопки левой колонки
        float twAddW = textRenderer->CalculateTextWidth("+ Волна", fBtn);
        textRenderer->RenderText("+ Волна", l.btnAddWavePos.x + (l.btnAddWaveSize.x - twAddW) * 0.5f, l.btnAddWavePos.y + (l.btnAddWaveSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(0.9f, 1.0f, 0.9f));

        float twDupW = textRenderer->CalculateTextWidth("Дублировать", fBtn);
        textRenderer->RenderText("Дублировать", l.btnDuplicateWavePos.x + (l.btnDuplicateWaveSize.x - twDupW) * 0.5f, l.btnDuplicateWavePos.y + (l.btnDuplicateWaveSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(0.85f, 0.95f, 1.0f));

        // Карточки списка волн
        for (int v = 0; v < maxWaves; ++v) {
            int wIdx = m_wavesScrollOffset + v;
            if (wIdx < 0 || wIdx >= static_cast<int>(waves.size())) break;

            float itemY = l.waveListStartY + static_cast<float>(v) * (l.waveItemH + l.waveItemGap);
            glm::vec2 itemPos(l.colLeftPos.x + std::clamp(6.0f * scale, 4.0f, 8.0f), itemY);
            glm::vec2 itemSize(itemW - delBtnW - moveBtnW * 2.0f - btnGap * 3.0f, l.waveItemH);

            glm::vec2 upPos(itemPos.x + itemSize.x + btnGap, itemY);
            glm::vec2 upSize(moveBtnW, l.waveItemH);

            glm::vec2 downPos(upPos.x + moveBtnW + btnGap, itemY);
            glm::vec2 downSize(moveBtnW, l.waveItemH);

            glm::vec2 delPos(downPos.x + moveBtnW + btnGap, itemY);
            glm::vec2 delSize(delBtnW, l.waveItemH);

            bool isSel = (wIdx == m_selectedWaveIdx);
            glm::vec3 textColor = isSel ? glm::vec3(1.0f, 0.95f, 0.40f) : glm::vec3(0.90f);

            int mobsCount = 0;
            for (const auto& p : waves[wIdx].parts) mobsCount += p.count;

            std::string label = "Волна #" + std::to_string(wIdx + 1);
            std::string countLabel = std::to_string(mobsCount) + " моб.";

            textRenderer->RenderText(label, itemPos.x + 8.0f * scale, itemPos.y + (itemSize.y - fSub * 28.0f) * 0.5f + 2.0f, fSub, textColor);
            float countTw = textRenderer->CalculateTextWidth(countLabel, fSub * 0.88f);
            textRenderer->RenderText(countLabel, itemPos.x + itemSize.x - countTw - 6.0f * scale, itemPos.y + (itemSize.y - fSub * 0.88f * 28.0f) * 0.5f + 2.0f, fSub * 0.88f, glm::vec3(0.65f, 0.75f, 0.85f));

            bool canMoveUp = (wIdx > 0);
            bool canMoveDown = (wIdx + 1 < static_cast<int>(waves.size()));

            float twUp = textRenderer->CalculateTextWidth("^", fBtn);
            textRenderer->RenderText("^", upPos.x + (upSize.x - twUp) * 0.5f, upPos.y + (upSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, canMoveUp ? glm::vec3(0.90f) : glm::vec3(0.35f));

            float twDown = textRenderer->CalculateTextWidth("v", fBtn);
            textRenderer->RenderText("v", downPos.x + (downSize.x - twDown) * 0.5f, downPos.y + (downSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, canMoveDown ? glm::vec3(0.90f) : glm::vec3(0.35f));

            float delTw = textRenderer->CalculateTextWidth("x", fBtn);
            textRenderer->RenderText("x", delPos.x + (delSize.x - delTw) * 0.5f, delPos.y + (delSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(1.0f, 0.65f, 0.65f));
        }

        // Правая колонка
        if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
            const auto& curWave = waves[m_selectedWaveIdx];

            // Шапка правой колонки
            std::string wNumStr = "ВОЛНА #" + std::to_string(m_selectedWaveIdx + 1);
            float headerTextY = l.btnAddPartPos.y + (l.btnAddPartSize.y - fTitle * 28.0f) * 0.5f + 2.0f;
            textRenderer->RenderText(wNumStr, l.colRightPos.x + 8.0f * scale, headerTextY, fTitle, glm::vec3(0.95f, 0.95f, 0.95f));

            float twNum = textRenderer->CalculateTextWidth(wNumStr, fTitle);
            std::string statsStr = "Время: ~" + formatFloat1(waveTotalTime) + "s  |  Врагов: " + std::to_string(totalMobsInWave);
            float statsTextY = l.btnAddPartPos.y + (l.btnAddPartSize.y - fSub * 28.0f) * 0.5f + 2.0f;
            textRenderer->RenderText(statsStr, l.colRightPos.x + twNum + 24.0f * scale, statsTextY, fSub, glm::vec3(0.40f, 0.95f, 0.70f));

            float twAddPart = textRenderer->CalculateTextWidth("+ Добавить пачку", fBtn);
            textRenderer->RenderText("+ Добавить пачку", l.btnAddPartPos.x + (l.btnAddPartSize.x - twAddPart) * 0.5f, l.btnAddPartPos.y + (l.btnAddPartSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(0.9f, 1.0f, 0.9f));

            // Содержимое карточек пачек
            if (curWave.parts.empty()) {
                textRenderer->RenderText("В этой волне нет настроенных пачек.", l.colRightPos.x + 20.0f * scale, l.cardsStartY + 28.0f * scale, fSub * 1.1f, glm::vec3(0.7f));
                textRenderer->RenderText("Нажмите '+ Добавить пачку' для настройки спавна мобов!", l.colRightPos.x + 20.0f * scale, l.cardsStartY + 54.0f * scale, fSub, glm::vec3(0.35f, 0.85f, 1.0f));
            } else {
                bool showBlinkCursor = (m_cursorBlinkTimer < 0.53f);

                for (int v = 0; v < l.maxVisibleParts; ++v) {
                    int pIdx = m_wavePartsScrollOffset + v;
                    if (pIdx >= static_cast<int>(curWave.parts.size())) break;
                    const auto& part = curWave.parts[pIdx];

                    glm::vec2 cardPos(l.colRightPos.x, l.cardsStartY + static_cast<float>(v) * (l.cardH + l.cardGap));
                    glm::vec2 cardSize(l.colRightSize.x, l.cardH);

                    // Бейдж номера пачки
                    std::string badge = "#" + std::to_string(pIdx + 1);
                    textRenderer->RenderText(badge, cardPos.x + 10.0f * scale, cardPos.y + 11.0f * scale, fSub * 1.1f, glm::vec3(0.35f, 0.85f, 1.0f));

                    // Кнопка типа врага
                    glm::vec2 typeBtnPos(cardPos.x + 40.0f * scale, cardPos.y + 7.0f * scale);
                    glm::vec2 typeBtnSize(130.0f * scale, 26.0f * scale);
                    std::string displayType = part.type;
                    glm::vec3 typeTxtCol(0.4f, 0.9f, 1.0f);
                    for (const auto& ch : getAllEnemyTypeInfo()) {
                        if (part.type == ch.id) {
                            displayType = ch.label;
                            typeTxtCol = ch.color;
                            break;
                        }
                    }
                    std::string typeLabel = displayType + (m_openDropdownPartIdx == pIdx ? "  ^" : "  v");
                    float twType = textRenderer->CalculateTextWidth(typeLabel, fBtn);
                    textRenderer->RenderText(typeLabel, typeBtnPos.x + (typeBtnSize.x - twType) * 0.5f, typeBtnPos.y + (typeBtnSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, typeTxtCol);

                    // Кнопка Spawner ID
                    glm::vec2 spawnerBtnPos(typeBtnPos.x + typeBtnSize.x + 8.0f * scale, cardPos.y + 7.0f * scale);
                    glm::vec2 spawnerBtnSize(130.0f * scale, 26.0f * scale);
                    std::string spLabel = getSpawnerLabel(part.spawnerId);
                    glm::vec3 spColor = getSpawnerColor(part.spawnerId);
                    float twSp = textRenderer->CalculateTextWidth(spLabel, fBtn);
                    textRenderer->RenderText(spLabel, spawnerBtnPos.x + (spawnerBtnSize.x - twSp) * 0.5f, spawnerBtnPos.y + (spawnerBtnSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::min(spColor * 1.25f, glm::vec3(1.0f)));

                    // Сводка пачки
                    float partDur = (part.count > 0 ? (part.count - 1) * part.spawnInterwal : 0.0f) + part.delayAfter;
                    std::string partSummary = std::to_string(part.count) + " моб.  (~" + formatFloat1(partDur) + "s)";
                    textRenderer->RenderText(partSummary, spawnerBtnPos.x + spawnerBtnSize.x + 14.0f * scale, cardPos.y + 12.0f * scale, fSub * 0.95f, glm::vec3(0.75f, 0.80f, 0.90f));

                    float moveBtnW = std::clamp(22.0f * scale, 18.0f, 26.0f);
                    float delBtnW = std::clamp(24.0f * scale, 20.0f, 26.0f);
                    float btnGap = 3.0f * scale;

                    glm::vec2 delPartBtnPos(cardPos.x + cardSize.x - delBtnW - 4.0f * scale, cardPos.y + 7.0f * scale);
                    glm::vec2 delPartBtnSize(delBtnW, 26.0f * scale);

                    glm::vec2 downPartBtnPos(delPartBtnPos.x - moveBtnW - btnGap, cardPos.y + 7.0f * scale);
                    glm::vec2 downPartBtnSize(moveBtnW, 26.0f * scale);

                    glm::vec2 upPartBtnPos(downPartBtnPos.x - moveBtnW - btnGap, cardPos.y + 7.0f * scale);
                    glm::vec2 upPartBtnSize(moveBtnW, 26.0f * scale);

                    bool canPartUp = (pIdx > 0);
                    bool canPartDown = (pIdx + 1 < static_cast<int>(curWave.parts.size()));

                    float twPUp = textRenderer->CalculateTextWidth("^", fBtn);
                    textRenderer->RenderText("^", upPartBtnPos.x + (upPartBtnSize.x - twPUp) * 0.5f, upPartBtnPos.y + (upPartBtnSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, canPartUp ? glm::vec3(0.90f) : glm::vec3(0.35f));

                    float twPDown = textRenderer->CalculateTextWidth("v", fBtn);
                    textRenderer->RenderText("v", downPartBtnPos.x + (downPartBtnSize.x - twPDown) * 0.5f, downPartBtnPos.y + (downPartBtnSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, canPartDown ? glm::vec3(0.90f) : glm::vec3(0.35f));

                    // Удаление пачки
                    float twDelPart = textRenderer->CalculateTextWidth("x", fBtn);
                    textRenderer->RenderText("x", delPartBtnPos.x + (delPartBtnSize.x - twDelPart) * 0.5f, delPartBtnPos.y + (delPartBtnSize.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(1.0f, 0.70f, 0.70f));

                    // Ряд 2: Контролы параметров
                    Row2Layout r2 = computeRow2Layout(cardPos, cardSize.x, scale, textRenderer);

                    // Count
                    textRenderer->RenderText("Кол-во:", r2.labelCountPos.x, r2.labelCountPos.y, fSub, glm::vec3(0.85f));
                    float twM = textRenderer->CalculateTextWidth("-", fBtn * 1.1f);
                    float mY = r2.cntDecPos.y + (r2.nudgeSize.y - fBtn * 1.1f * 28.0f) * 0.5f + 2.0f;
                    textRenderer->RenderText("-", r2.cntDecPos.x + (r2.nudgeSize.x - twM) * 0.5f, mY, fBtn * 1.1f, glm::vec3(1.0f));

                    bool isCntFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Count);
                    std::string cntStr = isCntFocused ? (m_inputText + (showBlinkCursor ? "|" : "")) : std::to_string(part.count);
                    glm::vec3 cntCol = isCntFocused ? glm::vec3(0.4f, 1.0f, 1.0f) : glm::vec3(1.0f, 0.95f, 0.4f);
                    float twCnt = textRenderer->CalculateTextWidth(cntStr, fSub);
                    float cntY = r2.cntBoxPos.y + (r2.cntBoxSize.y - fSub * 28.0f) * 0.5f + 2.0f;
                    textRenderer->RenderText(cntStr, r2.cntBoxPos.x + (r2.cntBoxSize.x - twCnt) * 0.5f, cntY, fSub, cntCol);

                    float twP = textRenderer->CalculateTextWidth("+", fBtn * 1.1f);
                    float pY = r2.cntIncPos.y + (r2.nudgeSize.y - fBtn * 1.1f * 28.0f) * 0.5f + 2.0f;
                    textRenderer->RenderText("+", r2.cntIncPos.x + (r2.nudgeSize.x - twP) * 0.5f, pY, fBtn * 1.1f, glm::vec3(1.0f));

                    // Interval
                    textRenderer->RenderText("Интервал:", r2.labelIntervalPos.x, r2.labelIntervalPos.y, fSub, glm::vec3(0.85f));
                    textRenderer->RenderText("-", r2.rateDecPos.x + (r2.nudgeSize.x - twM) * 0.5f, mY, fBtn * 1.1f, glm::vec3(1.0f));

                    bool isRateFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Interval);
                    std::string rateStr = isRateFocused ? (m_inputText + (showBlinkCursor ? "|" : "")) : (formatFloat1(part.spawnInterwal) + "s");
                    glm::vec3 rateCol = isRateFocused ? glm::vec3(0.4f, 1.0f, 1.0f) : glm::vec3(0.4f, 1.0f, 0.6f);
                    float twRate = textRenderer->CalculateTextWidth(rateStr, fSub);
                    float rateY = r2.rateBoxPos.y + (r2.rateBoxSize.y - fSub * 28.0f) * 0.5f + 2.0f;
                    textRenderer->RenderText(rateStr, r2.rateBoxPos.x + (r2.rateBoxSize.x - twRate) * 0.5f, rateY, fSub, rateCol);

                    textRenderer->RenderText("+", r2.rateIncPos.x + (r2.nudgeSize.x - twP) * 0.5f, pY, fBtn * 1.1f, glm::vec3(1.0f));

                    // Pause
                    textRenderer->RenderText("Пауза:", r2.labelPausePos.x, r2.labelPausePos.y, fSub, glm::vec3(0.85f));
                    textRenderer->RenderText("-", r2.pauseDecPos.x + (r2.nudgeSize.x - twM) * 0.5f, mY, fBtn * 1.1f, glm::vec3(1.0f));

                    bool isPauseFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Delay);
                    std::string pauseStr = isPauseFocused ? (m_inputText + (showBlinkCursor ? "|" : "")) : (formatFloat1(part.delayAfter) + "s");
                    glm::vec3 pauseCol = isPauseFocused ? glm::vec3(0.4f, 1.0f, 1.0f) : glm::vec3(1.0f, 0.8f, 0.4f);
                    float twPause = textRenderer->CalculateTextWidth(pauseStr, fSub);
                    float pauseY = r2.pauseBoxPos.y + (r2.pauseBoxSize.y - fSub * 28.0f) * 0.5f + 2.0f;
                    textRenderer->RenderText(pauseStr, r2.pauseBoxPos.x + (r2.pauseBoxSize.x - twPause) * 0.5f, pauseY, fSub, pauseCol);

                    textRenderer->RenderText("+", r2.pauseIncPos.x + (r2.nudgeSize.x - twP) * 0.5f, pY, fBtn * 1.1f, glm::vec3(1.0f));
                }
            }
        }

        // Футер
        std::string footerText = "Волн: " + std::to_string(waves.size()) + " | Выбрана: Волна #" + std::to_string(m_selectedWaveIdx + 1) + " | [Tab] След. поле | [Enter] Применить | [Esc] Закрыть";
        textRenderer->RenderText(footerText, l.modalPos.x + 16.0f * scale, footerY + (l.footerH - fSub * 28.0f) * 0.5f + 2.0f, fSub, glm::vec3(0.70f, 0.75f, 0.85f));
    }

    // 7. СЛОЙ ВЫПАДАЮЩЕГО СПИСКА ВЫБОРА МОБА
    if (m_openDropdownPartIdx >= 0 && m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
        const WaveConfig& curWave = waves[m_selectedWaveIdx];
        int visIdx = m_openDropdownPartIdx - m_wavePartsScrollOffset;
        if (visIdx >= 0 && visIdx < l.maxVisibleParts && m_openDropdownPartIdx < static_cast<int>(curWave.parts.size())) {
            glm::vec2 cardPos(l.colRightPos.x, l.cardsStartY + static_cast<float>(visIdx) * (l.cardH + l.cardGap));
            glm::vec2 typeBtnPos(cardPos.x + 40.0f * scale, cardPos.y + 7.0f * scale);
            glm::vec2 typeBtnSize(130.0f * scale, 26.0f * scale);

            float dropX = typeBtnPos.x;
            float dropW = 145.0f * scale;
            float itemH = std::clamp(28.0f * scale, 22.0f, 32.0f);
            const auto& choices = getEnemyTypeChoices();
            float totalDropH = itemH * static_cast<float>(choices.size());
            float dropY = typeBtnPos.y + typeBtnSize.y + 2.0f;
            if (dropY + totalDropH > l.modalPos.y + l.modalSize.y - l.footerH) {
                dropY = typeBtnPos.y - totalDropH - 2.0f;
            }

            renderer->drawSprite(whiteTexture, glm::vec2(dropX - 3.0f, dropY - 2.0f), glm::vec2(dropW + 6.0f, totalDropH + 4.0f), 0.0f, glm::vec3(0.02f, 0.03f, 0.05f));
            renderer->drawSprite(whiteTexture, glm::vec2(dropX - 1.0f, dropY - 1.0f), glm::vec2(dropW + 2.0f, totalDropH + 2.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
            renderer->drawSprite(whiteTexture, glm::vec2(dropX, dropY), glm::vec2(dropW, totalDropH), 0.0f, glm::vec3(0.10f, 0.12f, 0.17f));

            for (size_t c = 0; c < choices.size(); ++c) {
                bool hov = isPointInRect(mousePos, glm::vec2(dropX, dropY + itemH * static_cast<float>(c)), glm::vec2(dropW, itemH));
                bool isSelected = (curWave.parts[m_openDropdownPartIdx].type == choices[c].id);
                glm::vec3 bg = hov ? (choices[c].baseBg * 1.4f) : (isSelected ? choices[c].baseBg : glm::vec3(0.11f, 0.13f, 0.18f));
                renderer->drawSprite(whiteTexture, glm::vec2(dropX + 1.0f, dropY + itemH * static_cast<float>(c) + 1.0f), glm::vec2(dropW - 2.0f, itemH - 2.0f), 0.0f, bg);
                if (c > 0) {
                    renderer->drawSprite(whiteTexture, glm::vec2(dropX + 4.0f, dropY + itemH * static_cast<float>(c)), glm::vec2(dropW - 8.0f, 1.0f), 0.0f, glm::vec3(0.25f, 0.30f, 0.40f));
                }
            }

            renderer->flush();

            if (textRenderer) {
                float fBtn = std::clamp(0.46f * scale, 0.38f, 0.56f);
                for (size_t c = 0; c < choices.size(); ++c) {
                    textRenderer->RenderText(choices[c].label, dropX + 10.0f * scale, dropY + itemH * static_cast<float>(c) + (itemH - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, choices[c].color);
                }
            }
        }
    }
}

void EditorWaveModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer, float screenWidth, float screenHeight, const LevelMapData& mapData) {
    render(renderer, textRenderer, nullptr, static_cast<int>(screenWidth), static_cast<int>(screenHeight),
           48.0f, 64.0f, glm::vec2(-1000.0f), mapData.waves);
}

