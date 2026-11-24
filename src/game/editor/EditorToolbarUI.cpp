#include "EditorToolbarUI.h"
#include "../../renderer/BackgroundRenderer.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include "../../resources/ResourceManager.h"
#include "../ui/UICommon.h"
#include "../core/LocalizationManager.h"
#include <algorithm>
#include <cmath>

bool EditorToolbarUI::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

glm::vec3 EditorToolbarUI::getIdColor(int id) {
    switch (id) {
        case -1: return glm::vec3(1.0f, 0.85f, 0.25f); // Золотистый (Auto/Nearest)
        case 0:  return glm::vec3(0.20f, 0.85f, 1.0f); // Бирюзовый / Циан
        case 1:  return glm::vec3(1.0f, 0.55f, 0.15f); // Оранжевый / Янтарный
        case 2:  return glm::vec3(0.25f, 1.0f, 0.40f); // Лаймовый / Зеленый
        case 3:  return glm::vec3(1.0f, 0.35f, 0.85f); // Маджента / Розовый
        case 4:  return glm::vec3(0.95f, 0.30f, 0.30f); // Кораллово-красный
        default: return glm::vec3(0.70f, 0.70f, 0.95f); // Лавандовый
    }
}

float EditorToolbarUI::calculateTopBarHeight(int width, int height) {
    float scale = GetUIScale(width, height);
    return std::clamp(42.0f * scale, 36.0f, 56.0f);
}

float EditorToolbarUI::calculateBottomDockHeight(int width, int height) {
    float scale = GetUIScale(width, height);
    float dockH = std::clamp(54.0f * scale, 46.0f, 74.0f);
    float tabH = std::clamp(28.0f * scale, 24.0f, 34.0f);
    return dockH + tabH;
}

void EditorToolbarUI::init() {
}

void EditorToolbarUI::update(float dt) {
    (void)dt;
}

bool EditorToolbarUI::isHovered(float mouseX, float mouseY) const {
    if (mouseY < m_topBarH) return true;
    if (mouseY >= static_cast<float>(m_screenHeight) - m_dockH) return true;
    glm::vec2 p(mouseX, mouseY);
    for (const auto& btn : m_tabButtons) {
        if (isPointInRect(p, btn.pos, btn.size)) return true;
    }
    return false;
}

void EditorToolbarUI::updateLayout(TextRenderer* textRenderer, const EditorContext& ctx) {
    m_screenWidth = ctx.screenWidth;
    m_screenHeight = ctx.screenHeight;

    float scale = GetUIScale(ctx.screenWidth, ctx.screenHeight);
    m_topBarH = std::clamp(42.0f * scale, 36.0f, 56.0f);
    m_dockH = std::clamp(54.0f * scale, 46.0f, 74.0f);
    m_tabH = std::clamp(28.0f * scale, 24.0f, 34.0f);

    m_bottomButtons.clear();
    m_topButtons.clear();
    m_tabButtons.clear();

    auto getTextW = [textRenderer](const std::string& str, float sc) -> float {
        if (textRenderer) return textRenderer->CalculateTextWidth(str, sc);
        return static_cast<float>(str.length()) * 8.5f * sc;
    };

    // 1. КНОПКИ ВЕРХНЕГО ХЕДЕРА
    float topH = m_topBarH - 8.0f;
    float topY = 4.0f;
    float topPadding = std::clamp(5.0f * scale, 3.0f, 8.0f);
    float topFontScale = std::clamp(0.48f * scale, 0.38f, 0.62f);
    m_topFontScale = topFontScale;

    // Определяем режим сжатия хедера (если экран < 1360px)
    float screenW = static_cast<float>(ctx.screenWidth);
    bool isCompact = (screenW < 1360.0f);
    bool isUltraCompact = (screenW < 1120.0f);

    if (isCompact) {
        topFontScale = std::clamp(topFontScale * 0.92f, 0.38f, 0.52f);
        m_topFontScale = topFontScale;
        topPadding = std::clamp(topPadding * 0.70f, 3.0f, 5.0f);
    }
    if (isUltraCompact) {
        topFontScale = std::clamp(topFontScale * 0.88f, 0.35f, 0.46f);
        m_topFontScale = topFontScale;
        topPadding = 2.5f;
    }

    // Слева: заголовок "MAP EDITOR"
    float titleFontScale = std::clamp(0.80f * scale, 0.58f, 0.95f);
    if (isCompact) titleFontScale *= 0.90f;
    float titleW = getTextW("MAP EDITOR", titleFontScale);
    float leftX = 12.0f + titleW + (isCompact ? 8.0f : 14.0f);

    // btnHelp: [ ? ] - компактная кнопка справки и хоткеев
    float helpW = std::max(std::clamp(28.0f * scale, 24.0f, 36.0f), getTextW("[ ? ]", topFontScale) + 8.0f);
    EditorButton btnHelp;
    btnHelp.pos = glm::vec2(leftX, topY);
    btnHelp.size = glm::vec2(helpW, topH);
    btnHelp.label = "[ ? ]";
    btnHelp.isAction = true;
    btnHelp.actionId = 22; // Toggle Help Modal
    m_topButtons.push_back(btnHelp);
    leftX += helpW + topPadding;

    // btnCurrentMap: Отображение текущего имени карты
    float nameW = getTextW(ctx.currentLevelDisplayName, topFontScale);
    float maxMapBtnW = isUltraCompact ? 90.0f : (isCompact ? 130.0f : 180.0f);
    float mapBtnW = std::clamp(nameW + 16.0f, 65.0f, maxMapBtnW);
    EditorButton btnCurrentMap;
    btnCurrentMap.pos = glm::vec2(leftX, topY);
    btnCurrentMap.size = glm::vec2(mapBtnW, topH);
    btnCurrentMap.label = ctx.currentLevelDisplayName;
    btnCurrentMap.isAction = true;
    btnCurrentMap.actionId = 16; // Открыть список карт
    m_topButtons.push_back(btnCurrentMap);
    leftX += mapBtnW + topPadding;

    // btnRename
    std::string renLabel = isCompact ? "[ Имя ]" : LOC("LEVEL_BTN_RENAME");
    float renW = std::max(isCompact ? 42.0f : 52.0f, getTextW(renLabel, topFontScale) + (isCompact ? 10.0f : 16.0f));
    EditorButton btnRename;
    btnRename.pos = glm::vec2(leftX, topY);
    btnRename.size = glm::vec2(renW, topH);
    btnRename.label = renLabel;
    btnRename.isAction = true;
    btnRename.actionId = 14; // Переименовать
    m_topButtons.push_back(btnRename);
    leftX += renW + topPadding;

    // btnType
    std::string typeLabel = isCompact ? (ctx.isCampaign ? "[ Камп ]" : "[ Тест ]")
                                      : (ctx.isCampaign ? LOC("EDITOR_TYPE_CAMPAIGN") : LOC("EDITOR_TYPE_TEST"));
    float typeW = std::max(isCompact ? 56.0f : 76.0f, getTextW(typeLabel, topFontScale) + (isCompact ? 12.0f : 18.0f));
    EditorButton btnType;
    btnType.pos = glm::vec2(leftX, topY);
    btnType.size = glm::vec2(typeW, topH);
    btnType.label = typeLabel;
    btnType.isAction = true;
    btnType.actionId = 17; // Переключить Кампания/Тест
    m_topButtons.push_back(btnType);
    leftX += typeW + topPadding;

    // btnNewMap
    std::string newLabel = isCompact ? "[ + ]" : LOC("EDITOR_NEW_MAP");
    float newW = std::max(isCompact ? 30.0f : 66.0f, getTextW(newLabel, topFontScale) + (isCompact ? 10.0f : 18.0f));
    EditorButton btnNewMap;
    btnNewMap.pos = glm::vec2(leftX, topY);
    btnNewMap.size = glm::vec2(newW, topH);
    btnNewMap.label = newLabel;
    btnNewMap.isAction = true;
    btnNewMap.actionId = 15; // Создать новую карту
    m_topButtons.push_back(btnNewMap);
    leftX += newW + topPadding;

    // btnMaps
    std::string mapsLabel = isCompact ? "[ Карты ]" : LOC("EDITOR_MAPS_LIST");
    float mapsW = std::max(isCompact ? 52.0f : 60.0f, getTextW(mapsLabel, topFontScale) + (isCompact ? 12.0f : 18.0f));
    EditorButton btnMaps;
    btnMaps.pos = glm::vec2(leftX, topY);
    btnMaps.size = glm::vec2(mapsW, topH);
    btnMaps.label = mapsLabel;
    btnMaps.isAction = true;
    btnMaps.actionId = 16; // Список карт
    m_topButtons.push_back(btnMaps);
    leftX += mapsW + topPadding;

    // Кнопка модального окна свойств уровня (EditorLevelSettingsModal)
    std::string settingsLabel = isCompact ? "Свойства" : "Свойства карты";
    float settingsW = std::max(isCompact ? 80.0f : 110.0f, getTextW(settingsLabel, topFontScale) + (isCompact ? 14.0f : 20.0f));
    EditorButton btnLevelSettings;
    btnLevelSettings.pos = glm::vec2(leftX, topY);
    btnLevelSettings.size = glm::vec2(settingsW, topH);
    btnLevelSettings.label = settingsLabel;
    btnLevelSettings.tooltip = "Свойства карты: Деньги, HP, Башни, Тир апгрейдов";
    btnLevelSettings.isAction = true;
    btnLevelSettings.actionId = 24; // Свойства карты
    m_topButtons.push_back(btnLevelSettings);
    leftX += settingsW + topPadding;

    m_topBarLeftEndX = leftX;

    // Справа: Выбор камеры, выбор фона и выбор размера карты
    std::string camLabel;
    if (isUltraCompact) {
        camLabel = ctx.cameraConfigMode ? "[ Кам: Руч* ]" : (ctx.cameraIsCustom ? "[ Кам: Руч ]" : "[ Кам: Авто ]");
    } else if (isCompact) {
        camLabel = ctx.cameraConfigMode ? "[ Кам: Руч* ]" : (ctx.cameraIsCustom ? "[ Кам: Руч ]" : "[ Кам: Авто ]");
    } else {
        camLabel = ctx.cameraConfigMode ? "[ Камера: Ручная* ]" : (ctx.cameraIsCustom ? "[ Камера: Ручная ]" : "[ Камера: Авто ]");
    }
    float camBtnW = getTextW(camLabel, topFontScale) + (isCompact ? 10.0f : 16.0f);

    std::string lockLabel = isUltraCompact ? "[ Фикс ]" : (isCompact ? "[ Фикс. камеру ]" : "[ Зафиксировать камеру ]");
    float lockBtnW = getTextW(lockLabel, topFontScale) + (isCompact ? 10.0f : 16.0f);

    std::string resetLabel = isCompact ? "[ Сброс ]" : "[ Сброс в Авто ]";
    float resetBtnW = getTextW(resetLabel, topFontScale) + (isCompact ? 10.0f : 14.0f);

    std::string bgLabel = BackgroundRenderer::getEditorToolbarBgLabel(ctx.background, isCompact);
    float bgBtnW = getTextW(bgLabel, topFontScale) + (isCompact ? 10.0f : 16.0f);

    std::string hudLabel = "[ HUD: Выкл ]";
    if (ctx.hudPreviewMode == HudPreviewMode::Scale100) hudLabel = "[ HUD: 100% ]";
    else if (ctx.hudPreviewMode == HudPreviewMode::Scale125) hudLabel = "[ HUD: 125% ]";
    else if (ctx.hudPreviewMode == HudPreviewMode::Scale150) hudLabel = "[ HUD: 150% ]";
    float hudBtnW = getTextW(hudLabel, topFontScale) + (isCompact ? 10.0f : 16.0f);

    float presetW = std::clamp(isCompact ? 68.0f * scale : 80.0f * scale, 56.0f, 96.0f);
    float decIncW = std::clamp(isCompact ? 24.0f * scale : 30.0f * scale, 22.0f, 36.0f);

    float rightGroupW = camBtnW + topPadding
                      + (ctx.cameraConfigMode ? (lockBtnW + topPadding) : 0.0f)
                      + (ctx.cameraIsCustom ? (resetBtnW + topPadding) : 0.0f)
                      + hudBtnW + topPadding
                      + bgBtnW + topPadding + presetW + topPadding + (decIncW + topPadding) * 4.0f;

    // Гарантированный зазор между левым и правым блоком тулбара (наезды физически невозможны!)
    float minCenterGap = std::clamp(16.0f * scale, 12.0f, 24.0f);
    float currentTopX = std::max(leftX + minCenterGap, screenW - rightGroupW - 12.0f);

    EditorButton btnCamera;
    btnCamera.pos = glm::vec2(currentTopX, topY);
    btnCamera.size = glm::vec2(camBtnW, topH);
    btnCamera.label = camLabel;
    btnCamera.isAction = true;
    btnCamera.actionId = 19; // Toggle Camera mode
    m_topButtons.push_back(btnCamera);
    currentTopX += camBtnW + topPadding;

    if (ctx.cameraConfigMode) {
        EditorButton btnLock;
        btnLock.pos = glm::vec2(currentTopX, topY);
        btnLock.size = glm::vec2(lockBtnW, topH);
        btnLock.label = lockLabel;
        btnLock.isAction = true;
        btnLock.actionId = 20; // Lock Camera
        m_topButtons.push_back(btnLock);
        currentTopX += lockBtnW + topPadding;
    }

    if (ctx.cameraIsCustom) {
        EditorButton btnReset;
        btnReset.pos = glm::vec2(currentTopX, topY);
        btnReset.size = glm::vec2(resetBtnW, topH);
        btnReset.label = resetLabel;
        btnReset.isAction = true;
        btnReset.actionId = 21; // Reset Camera to Auto
        m_topButtons.push_back(btnReset);
        currentTopX += resetBtnW + topPadding;
    }

    EditorButton btnHudPreview;
    btnHudPreview.pos = glm::vec2(currentTopX, topY);
    btnHudPreview.size = glm::vec2(hudBtnW, topH);
    btnHudPreview.label = hudLabel;
    btnHudPreview.isAction = true;
    btnHudPreview.actionId = 23; // Toggle HUD preview
    m_topButtons.push_back(btnHudPreview);
    currentTopX += hudBtnW + topPadding;

    EditorButton btnBackground;
    btnBackground.pos = glm::vec2(currentTopX, topY);
    btnBackground.size = glm::vec2(bgBtnW, topH);
    btnBackground.label = bgLabel;
    btnBackground.isAction = true;
    btnBackground.actionId = 18; // Toggle background
    m_topButtons.push_back(btnBackground);
    currentTopX += bgBtnW + topPadding;

    EditorButton btnPreset;
    btnPreset.pos = glm::vec2(currentTopX, topY);
    btnPreset.size = glm::vec2(presetW, topH);
    btnPreset.label = std::to_string(ctx.gridWidth) + "x" + std::to_string(ctx.gridHeight);
    btnPreset.isAction = true;
    btnPreset.actionId = 8; // Preset cycle
    m_topButtons.push_back(btnPreset);
    currentTopX += presetW + topPadding;

    EditorButton btnWDec;
    btnWDec.pos = glm::vec2(currentTopX, topY);
    btnWDec.size = glm::vec2(decIncW, topH);
    btnWDec.label = "W-";
    btnWDec.tooltip = "W- (-1 справа, Alt: -1 слева)";
    btnWDec.isAction = true;
    btnWDec.actionId = 9; // W-
    m_topButtons.push_back(btnWDec);
    currentTopX += decIncW + topPadding;

    EditorButton btnWInc;
    btnWInc.pos = glm::vec2(currentTopX, topY);
    btnWInc.size = glm::vec2(decIncW, topH);
    btnWInc.label = "W+";
    btnWInc.tooltip = "W+ (+1 справа, Alt: +1 слева)";
    btnWInc.isAction = true;
    btnWInc.actionId = 10; // W+
    m_topButtons.push_back(btnWInc);
    currentTopX += decIncW + topPadding;

    EditorButton btnHDec;
    btnHDec.pos = glm::vec2(currentTopX, topY);
    btnHDec.size = glm::vec2(decIncW, topH);
    btnHDec.label = "H-";
    btnHDec.tooltip = "H- (-1 снизу, Alt: -1 сверху)";
    btnHDec.isAction = true;
    btnHDec.actionId = 11; // H-
    m_topButtons.push_back(btnHDec);
    currentTopX += decIncW + topPadding;

    EditorButton btnHInc;
    btnHInc.pos = glm::vec2(currentTopX, topY);
    btnHInc.size = glm::vec2(decIncW, topH);
    btnHInc.label = "H+";
    btnHInc.tooltip = "H+ (+1 снизу, Alt: +1 сверху)";
    btnHInc.isAction = true;
    btnHInc.actionId = 12; // H+
    m_topButtons.push_back(btnHInc);

    // 2. ВКЛАДКИ ПАЛИТРЫ (прямо над нижней панелью)
    float bottomFontScale = std::clamp(0.46f * scale, 0.36f, 0.60f);
    m_bottomFontScale = bottomFontScale;

    float tabY = (static_cast<float>(ctx.screenHeight) - m_dockH) - m_tabH + 2.0f;
    float tabCurrentX = 10.0f;
    float tabPadding = 6.0f;

    struct TabDef {
        PaletteCategory cat;
        std::string label;
        int actionId;
    };
    std::vector<TabDef> tabDefs = {
        { PaletteCategory::Tiles, "[1] " + LOC("EDITOR_TAB_TILES"), 101 },
        { PaletteCategory::SpecialObjects, "[2] " + LOC("EDITOR_TAB_OBJECTS"), 102 },
        { PaletteCategory::Particles, "[3] Партиклы", 103 },
        { PaletteCategory::Decorations, "[4] Декор", 104 }
    };

    for (const auto& td : tabDefs) {
        float tw = getTextW(td.label, bottomFontScale) + 24.0f;
        EditorButton tBtn;
        tBtn.pos = glm::vec2(tabCurrentX, tabY);
        tBtn.size = glm::vec2(tw, m_tabH);
        tBtn.label = td.label;
        tBtn.isAction = true;
        tBtn.actionId = td.actionId;
        m_tabButtons.push_back(tBtn);
        tabCurrentX += tw + tabPadding;
    }

    // 3. КНОПКИ НИЖНЕГО ТУЛБАРА
    float btnH = m_dockH - 12.0f;
    float bottomY = static_cast<float>(ctx.screenHeight) - m_dockH + 6.0f;
    float startX = 10.0f;
    float padding = std::clamp(4.0f * scale, 2.0f, 6.0f);

    struct BottomItem {
        std::string label;
        bool isAction;
        EditorBrush brush;
        int actionId;
        float width;
    };

    std::vector<BottomItem> items;
    size_t splitIndex1 = 0;
    size_t splitIndex2 = 0;

    if (m_currentCategory == PaletteCategory::Tiles) {
        // Категория Tiles: Земля/Асфальт/Штрек, Стена/Ящики/Киноварь, Платформа/Трава/Уступ, Дорога, Шурф, Ластик
        std::string groundLabel = "1:" + LOC("EDITOR_GROUND");
        if (ctx.background == "mine_entrance" || ctx.background == "mine" || BackgroundRenderer::isCinnabarMine(ctx.background) || ctx.background == "pipes_canal" || ctx.background == "channel") {
            groundLabel = "1:" + BackgroundRenderer::getEditorGroundName(ctx.background);
        }
        std::string wallLabel = "2:" + LOC("EDITOR_WALL");
        if (ctx.background == "mine_entrance" || ctx.background == "mine" || BackgroundRenderer::isCinnabarMine(ctx.background)) {
            wallLabel = "2:" + BackgroundRenderer::getEditorWallName(ctx.background);
        }
        std::string platformLabel = "3:" + LOC("EDITOR_PLATFORM");
        if (ctx.background == "mine_entrance" || ctx.background == "mine" || BackgroundRenderer::isCinnabarMine(ctx.background) || ctx.background == "pipes_canal" || ctx.background == "channel") {
            platformLabel = "3:" + BackgroundRenderer::getEditorPlatformName(ctx.background);
        }
        items.push_back({ groundLabel,     false, EditorBrush::Ground,    0, 0.0f });
        items.push_back({ wallLabel,       false, EditorBrush::Wall,      0, 0.0f });
        items.push_back({ platformLabel,   false, EditorBrush::Platform,  0, 0.0f });
        items.push_back({ "4:" + LOC("EDITOR_PATH"),       false, EditorBrush::Path,      0, 0.0f });
        items.push_back({ "5:" + LOC("EDITOR_CHASM"),      false, EditorBrush::Chasm,     0, 0.0f });
        items.push_back({ "6:Запрет",                       false, EditorBrush::NoBuild,   0, 0.0f });
        items.push_back({ "0:" + LOC("EDITOR_ERASER"),     false, EditorBrush::Eraser,    0, 0.0f });
        splitIndex1 = items.size();
    } else if (m_currentCategory == PaletteCategory::SpecialObjects) {
        // Категория SpecialObjects: Спавнер, База, Рельсы, Депо, Тупик, селектор ID [-] [ID:Auto] [+]
        items.push_back({ "1:" + LOC("EDITOR_SPAWNER"),    false, EditorBrush::Spawner,   0, 0.0f });
        items.push_back({ "2:" + LOC("EDITOR_BASE"),       false, EditorBrush::Base,      0, 0.0f });
        items.push_back({ "3:" + LOC("EDITOR_RAIL"),       false, EditorBrush::Rail,      0, 0.0f });
        items.push_back({ LOC("EDITOR_RAIL_START"),        false, EditorBrush::RailStart, 0, 0.0f });
        items.push_back({ LOC("EDITOR_RAIL_END"),          false, EditorBrush::RailEnd,   0, 0.0f });
        items.push_back({ "0:" + LOC("EDITOR_ERASER"),     false, EditorBrush::Eraser,    0, 0.0f });
        splitIndex1 = items.size();

        // ID блок
        items.push_back({ "[-]", true, EditorBrush::Wall, 5, 0.0f });
        std::string idText = (ctx.selectedId == -1) ? "ID:Auto" : ("ID:#" + std::to_string(ctx.selectedId));
        items.push_back({ idText, true, EditorBrush::Wall, 6, 0.0f });
        items.push_back({ "[+]", true, EditorBrush::Wall, 7, 0.0f });
        splitIndex2 = items.size();
    } else if (m_currentCategory == PaletteCategory::Particles) {
        // Категория Particles: Пресеты эмиттеров частиц
        items.push_back({ "1:Свищ пара",  false, EditorBrush::EmitterSteamJet,  0, 0.0f });
        items.push_back({ "2:Капель",     false, EditorBrush::EmitterWaterDrip, 0, 0.0f });
        items.push_back({ "3:Искры",      false, EditorBrush::EmitterSparks,    0, 0.0f });
        items.push_back({ "4:Дым",        false, EditorBrush::EmitterSmoke,     0, 0.0f });
        items.push_back({ "5:Туман",      false, EditorBrush::EmitterFog,       0, 0.0f });
        items.push_back({ "0:" + LOC("EDITOR_ERASER"), false, EditorBrush::Eraser, 0, 0.0f });
        splitIndex1 = items.size();
    } else {
        // Категория Decorations: Пагинация декораций (Стр. 1/2 и 2/2)
        // Стрелка [ < ] перехода на предыдущую страницу
        items.push_back({ "<", true, EditorBrush::Wall, 26, 0.0f });

        if (m_decorPage == 0) {
            // Страница 1: Классические декорации двора (10 слотов)
            items.push_back({ "1:Куст",       false, EditorBrush::DecorBush,       0, 0.0f });
            items.push_back({ "2:Пучок",      false, EditorBrush::DecorGrass,      0, 0.0f });
            items.push_back({ "3:Поле",       false, EditorBrush::DecorGrassField, 0, 0.0f });
            items.push_back({ "4:Цветы",      false, EditorBrush::DecorFlower,     0, 0.0f });
            items.push_back({ "5:Камень",     false, EditorBrush::DecorStone,      0, 0.0f });
            items.push_back({ "6:Каска",      false, EditorBrush::DecorHelmet,     0, 0.0f });
            items.push_back({ "7:Кирка",      false, EditorBrush::DecorPickaxe,    0, 0.0f });
            items.push_back({ "8:Лужа",       false, EditorBrush::DecorPuddle,     0, 0.0f });
            items.push_back({ "9:Трещина",    false, EditorBrush::DecorCrack,      0, 0.0f });
            items.push_back({ "F:Туман",      false, EditorBrush::DecorFog,        0, 0.0f });
        } else {
            // Страница 2: Шахтный инвентарь и новые декорации рудника
            items.push_back({ "1:Ящик",       false, EditorBrush::DecorWoodenCrate, 0, 0.0f });
            items.push_back({ "2:Вагонетка",  false, EditorBrush::DecorBrokenCart,  0, 0.0f });
            items.push_back({ "3:Мусор",      false, EditorBrush::DecorScrapDebris, 0, 0.0f });
            items.push_back({ "4:Дерево",     false, EditorBrush::DecorDeadTree,    0, 0.0f });
            items.push_back({ "5:Бурьян",     false, EditorBrush::DecorDryGrass,    0, 0.0f });
            items.push_back({ "6:Руда",       false, EditorBrush::DecorOreDebris,   0, 0.0f });
        }

        // Стрелка [ > ] перехода на следующую страницу и индикатор страницы
        items.push_back({ ">", true, EditorBrush::Wall, 27, 0.0f });
        items.push_back({ (m_decorPage == 0 ? "1/2" : "2/2"), true, EditorBrush::Wall, 28, 0.0f });

        items.push_back({ "0:" + LOC("EDITOR_ERASER"), false, EditorBrush::Eraser, 0, 0.0f });
        items.push_back({ std::string("G:") + (ctx.decorGridSnap ? "Сетка ВКЛ" : "Сетка ВЫКЛ"), true, EditorBrush::Wall, 25, 0.0f });
        splitIndex1 = items.size();
    }

    // Действия (всегда доступны на панели)
    items.push_back({ LOC("EDITOR_WAVES"),          true, EditorBrush::Wall, 13, 0.0f });
    items.push_back({ LOC("EDITOR_MINECART_BTN"),   true, EditorBrush::Wall, 18, 0.0f });
    items.push_back({ LOC("EDITOR_SAVE"),           true, EditorBrush::Wall, 1,  0.0f });
    items.push_back({ LOC("EDITOR_TEST"),           true, EditorBrush::Wall, 2,  0.0f });
    items.push_back({ LOC("EDITOR_CLEAR"),          true, EditorBrush::Wall, 3,  0.0f });
    items.push_back({ LOC("EDITOR_EXIT"),           true, EditorBrush::Wall, 4,  0.0f });

    // Вычисляем ширину для каждого элемента с запасом по краям
    float totalWidth = startX;
    for (size_t i = 0; i < items.size(); ++i) {
        auto& it = items[i];
        float tw = getTextW(it.label, bottomFontScale);
        if (it.actionId == 5 || it.actionId == 7 || it.actionId == 26 || it.actionId == 27) {
            it.width = 28.0f;
        } else if (it.actionId == 28) {
            it.width = 38.0f;
        } else if (it.actionId == 6) {
            it.width = std::max(68.0f, tw + 18.0f);
        } else {
            it.width = std::max(56.0f, tw + 18.0f);
        }
        totalWidth += it.width + padding;
        if (i + 1 == splitIndex1 || (splitIndex2 > 0 && i + 1 == splitIndex2)) {
            totalWidth += 8.0f; // Разделители
        }
    }

    // Если всё вместе шире экрана, пропорционально уменьшаем
    float maxAvailW = static_cast<float>(ctx.screenWidth) - 20.0f;
    if (totalWidth > maxAvailW && totalWidth > 0.0f) {
        float factor = maxAvailW / totalWidth;
        bottomFontScale = std::max(0.36f, bottomFontScale * factor);
        m_bottomFontScale = bottomFontScale;
        padding = std::max(2.0f, padding * factor);
        for (auto& it : items) {
            float tw = getTextW(it.label, bottomFontScale);
            if (it.actionId == 5 || it.actionId == 7 || it.actionId == 26 || it.actionId == 27) {
                it.width = std::max(22.0f, 28.0f * factor);
            } else if (it.actionId == 28) {
                it.width = std::max(28.0f, 38.0f * factor);
            } else if (it.actionId == 6) {
                it.width = std::max(50.0f, tw + 10.0f);
            } else {
                it.width = std::max(44.0f, tw + 10.0f);
            }
        }
    }

    // Размещаем кнопки
    float currentX = startX;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i == splitIndex1 || (splitIndex2 > 0 && i == splitIndex2)) {
            currentX += 8.0f; // Визуальный разделитель
        }
        const auto& it = items[i];
        EditorButton btn;
        btn.pos = glm::vec2(currentX, bottomY);
        btn.size = glm::vec2(it.width, btnH);
        btn.label = it.label;
        btn.isAction = it.isAction;
        btn.brush = it.brush;
        btn.actionId = it.actionId;
        if (!btn.isAction && btn.brush == EditorBrush::NoBuild) {
            btn.tooltip = "Запрет стройки (ЛКМ - ставить, ПКМ - снять)";
        } else if (!btn.isAction && btn.brush == EditorBrush::Eraser) {
            btn.tooltip = "Ластик (Shift: снять только запрет стройки)";
        } else if (btn.actionId == 25) {
            btn.tooltip = "Привязка к сетке [G]: примагничивает декор к тайлам карты";
        } else if (btn.actionId == 26) {
            btn.tooltip = std::string("Стр. ") + (m_decorPage == 0 ? "1/2" : "2/2") + ": Пред. страница декора [Колесико мыши]";
        } else if (btn.actionId == 27) {
            btn.tooltip = std::string("Стр. ") + (m_decorPage == 0 ? "1/2" : "2/2") + ": След. страница декора [Колесико мыши]";
        } else if (btn.actionId == 28) {
            btn.tooltip = std::string("Страница ") + (m_decorPage == 0 ? "1 из 2" : "2 из 2") + " [Клик для смены страницы]";
        } else if (!btn.isAction && btn.brush == EditorBrush::DecorWoodenCrate) {
            btn.tooltip = "Ящик [1]: Старый деревянный ящик шахтеров";
        } else if (!btn.isAction && btn.brush == EditorBrush::DecorBrokenCart) {
            btn.tooltip = "Вагонетка [2]: Опрокинутая разбитая ртутная вагонетка";
        } else if (!btn.isAction && btn.brush == EditorBrush::DecorScrapDebris) {
            btn.tooltip = "Мусор/доски [3]: Обломки крепи, доски и металлолом";
        } else if (!btn.isAction && btn.brush == EditorBrush::DecorDeadTree) {
            btn.tooltip = "Сухое дерево [4]: Высокое мертвое ветвистое дерево без листьев";
        } else if (!btn.isAction && btn.brush == EditorBrush::DecorDryGrass) {
            btn.tooltip = "Сухая трава [5]: Пучок желто-бурого степного бурьяна";
        } else if (!btn.isAction && btn.brush == EditorBrush::DecorOreDebris) {
            btn.tooltip = "Обломки руды [6]: Каменная осыпь с вкраплениями темно-красной киновари";
        }
        m_bottomButtons.push_back(btn);
        currentX += it.width + padding;
    }
}

EditorAction EditorToolbarUI::handleInput(float mouseX, float mouseY, bool mousePressed, bool wasMousePressed, bool isShiftDown, const EditorContext& ctx, bool isAltDown) {
    (void)ctx;
    EditorAction result;
    if (!mousePressed || wasMousePressed) {
        return result;
    }

    glm::vec2 mousePos(mouseX, mouseY);
    int step = isShiftDown ? 5 : 1;

    // 1. Верхний бар
    for (const auto& btn : m_topButtons) {
        if (isPointInRect(mousePos, btn.pos, btn.size)) {
            if (btn.actionId == 8) {
                result.type = EditorActionType::CyclePreset;
            } else if (btn.actionId == 9) {
                result.type = EditorActionType::ResizeMap;
                result.intParam = -step;
                result.intParam2 = 0;
                result.isAltDown = isAltDown;
            } else if (btn.actionId == 10) {
                result.type = EditorActionType::ResizeMap;
                result.intParam = +step;
                result.intParam2 = 0;
                result.isAltDown = isAltDown;
            } else if (btn.actionId == 11) {
                result.type = EditorActionType::ResizeMap;
                result.intParam = 0;
                result.intParam2 = -step;
                result.isAltDown = isAltDown;
            } else if (btn.actionId == 12) {
                result.type = EditorActionType::ResizeMap;
                result.intParam = 0;
                result.intParam2 = +step;
                result.isAltDown = isAltDown;
            } else if (btn.actionId == 14) {
                result.type = EditorActionType::OpenRenameModal;
            } else if (btn.actionId == 15) {
                result.type = EditorActionType::NewMap;
            } else if (btn.actionId == 16) {
                result.type = EditorActionType::OpenMapsModal;
            } else if (btn.actionId == 17) {
                result.type = EditorActionType::ToggleCampaign;
            } else if (btn.actionId == 18) {
                result.type = EditorActionType::ToggleBackground;
            } else if (btn.actionId == 19) {
                result.type = EditorActionType::ToggleCameraMode;
            } else if (btn.actionId == 20) {
                result.type = EditorActionType::LockCamera;
            } else if (btn.actionId == 21) {
                result.type = EditorActionType::ResetCamera;
            } else if (btn.actionId == 22) {
                result.type = EditorActionType::ToggleHelpModal;
            } else if (btn.actionId == 23) {
                result.type = EditorActionType::ToggleHudPreview;
            } else if (btn.actionId == 24) { // Свойства карты
                result.type = EditorActionType::OpenLevelSettingsModal;
            }
            return result;
        }
    }

    // 2. Вкладки палитры
    for (const auto& btn : m_tabButtons) {
        if (isPointInRect(mousePos, btn.pos, btn.size)) {
            if (btn.actionId == 101) {
                m_currentCategory = PaletteCategory::Tiles;
            } else if (btn.actionId == 102) {
                m_currentCategory = PaletteCategory::SpecialObjects;
            } else if (btn.actionId == 103) {
                m_currentCategory = PaletteCategory::Particles;
            } else if (btn.actionId == 104) {
                m_currentCategory = PaletteCategory::Decorations;
            }
            result.type = EditorActionType::SwitchPaletteCategory;
            return result;
        }
    }

    // 3. Нижний тулбар
    for (const auto& btn : m_bottomButtons) {
        if (isPointInRect(mousePos, btn.pos, btn.size)) {
            if (!btn.isAction) {
                result.type = EditorActionType::SetBrush;
                result.brush = btn.brush;
            } else {
                if (btn.actionId == 1) {
                    result.type = EditorActionType::SaveMap;
                } else if (btn.actionId == 2) {
                    result.type = EditorActionType::TestMap;
                } else if (btn.actionId == 3) {
                    result.type = EditorActionType::ClearMap;
                } else if (btn.actionId == 4) {
                    result.type = EditorActionType::Exit;
                } else if (btn.actionId == 5) {
                    result.type = EditorActionType::CycleSelectedId;
                    result.intParam = -1;
                } else if (btn.actionId == 6 || btn.actionId == 7) {
                    result.type = EditorActionType::CycleSelectedId;
                    result.intParam = +1;
                } else if (btn.actionId == 13) {
                    result.type = EditorActionType::ToggleWavesModal;
                } else if (btn.actionId == 18) {
                    result.type = EditorActionType::ToggleMinecartModal;
                } else if (btn.actionId == 25) {
                    result.type = EditorActionType::ToggleDecorGridSnap;
                } else if (btn.actionId == 26) {
                    prevDecorPage();
                    result.type = EditorActionType::SwitchPaletteCategory;
                    return result;
                } else if (btn.actionId == 27) {
                    nextDecorPage();
                    result.type = EditorActionType::SwitchPaletteCategory;
                    return result;
                } else if (btn.actionId == 28) {
                    m_decorPage = (m_decorPage == 0) ? 1 : 0;
                    result.type = EditorActionType::SwitchPaletteCategory;
                    return result;
                }
            }
            return result;
        }
    }

    return result;
}

EditorAction EditorToolbarUI::handleInput(float mouseX, float mouseY, bool mousePressed, bool mouseReleased, EditorContext& ctx) {
    (void)mouseReleased;
    return handleInput(mouseX, mouseY, mousePressed, false, false, ctx);
}

void EditorToolbarUI::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                             const Texture2D* whiteTexture,
                             const EditorContext& ctx, glm::vec2 mousePos) {
    (void)mousePos;
    if (!renderer) return;

    float screenW = static_cast<float>(ctx.screenWidth);
    float screenH = static_cast<float>(ctx.screenHeight);

    // 1. ВЕРХНИЙ ХЕДЕР-БАР
    renderer->drawSprite(whiteTexture, glm::vec2(0.0f, 0.0f), glm::vec2(screenW, m_topBarH), 0.0f, glm::vec3(0.07f, 0.08f, 0.10f));
    renderer->drawSprite(whiteTexture, glm::vec2(0.0f, m_topBarH - 2.0f), glm::vec2(screenW, 2.0f), 0.0f, glm::vec3(0.25f, 0.27f, 0.32f));

    // Кнопки верхнего хедера
    for (const auto& btn : m_topButtons) {
        glm::vec3 btnBg = glm::vec3(0.16f, 0.18f, 0.23f);
        glm::vec3 borderColor = glm::vec3(0.32f, 0.36f, 0.44f);
        if (btn.actionId == 8) {
            borderColor = glm::vec3(0.3f, 0.8f, 1.0f);
        } else if (btn.actionId == 14) { // Rename
            borderColor = glm::vec3(1.0f, 0.85f, 0.3f);
            btnBg = glm::vec3(0.22f, 0.20f, 0.12f);
        } else if (btn.actionId == 15) { // +New
            borderColor = glm::vec3(0.3f, 0.9f, 0.4f);
            btnBg = glm::vec3(0.12f, 0.22f, 0.14f);
        } else if (btn.actionId == 16) { // Maps / MapName
            borderColor = glm::vec3(0.4f, 0.7f, 1.0f);
            btnBg = glm::vec3(0.14f, 0.20f, 0.28f);
        } else if (btn.actionId == 18) { // Background toggle
            if (BackgroundRenderer::isCinnabarMine(ctx.background)) {
                borderColor = glm::vec3(0.92f, 0.30f, 0.32f);
                btnBg = glm::vec3(0.25f, 0.11f, 0.13f);
            } else {
                borderColor = glm::vec3(0.35f, 0.85f, 0.45f);
                btnBg = glm::vec3(0.12f, 0.22f, 0.15f);
            }
        } else if (btn.actionId == 19) { // Camera mode toggle
            if (ctx.cameraConfigMode) {
                borderColor = glm::vec3(0.35f, 0.85f, 1.0f);
                btnBg = glm::vec3(0.12f, 0.26f, 0.36f);
            } else if (ctx.cameraIsCustom) {
                borderColor = glm::vec3(0.30f, 0.65f, 0.90f);
                btnBg = glm::vec3(0.11f, 0.18f, 0.26f);
            } else {
                borderColor = glm::vec3(0.35f, 0.40f, 0.48f);
                btnBg = glm::vec3(0.13f, 0.15f, 0.18f);
            }
        } else if (btn.actionId == 20) { // Lock Camera
            borderColor = glm::vec3(0.40f, 0.95f, 0.55f);
            btnBg = glm::vec3(0.12f, 0.28f, 0.16f);
        } else if (btn.actionId == 21) { // Reset Camera
            borderColor = glm::vec3(0.85f, 0.55f, 0.30f);
            btnBg = glm::vec3(0.25f, 0.16f, 0.10f);
        } else if (btn.actionId == 22) { // Help [ ? ]
            if (ctx.isHelpModalOpen) {
                borderColor = glm::vec3(0.40f, 0.85f, 1.0f);
                btnBg = glm::vec3(0.18f, 0.32f, 0.45f);
            } else {
                borderColor = glm::vec3(0.85f, 0.75f, 0.30f);
                btnBg = glm::vec3(0.22f, 0.18f, 0.12f);
            }
        } else if (btn.actionId == 23) { // HUD Preview
            if (ctx.hudPreviewMode != HudPreviewMode::None) {
                borderColor = glm::vec3(1.0f, 0.75f, 0.25f);
                btnBg = glm::vec3(0.25f, 0.20f, 0.10f);
            } else {
                borderColor = glm::vec3(0.45f, 0.45f, 0.50f);
                btnBg = glm::vec3(0.14f, 0.15f, 0.18f);
            }
        } else if (btn.actionId == 24) { // Свойства карты
            if (ctx.isLevelSettingsModalOpen) {
                borderColor = glm::vec3(1.0f, 0.85f, 0.35f);
                btnBg = glm::vec3(0.24f, 0.32f, 0.44f);
            } else {
                borderColor = glm::vec3(0.40f, 0.70f, 0.95f);
                btnBg = glm::vec3(0.14f, 0.20f, 0.28f);
            }
        }
        renderer->drawSprite(whiteTexture, btn.pos, btn.size, 0.0f, borderColor);
        renderer->drawSprite(whiteTexture, btn.pos + glm::vec2(2.0f), btn.size - glm::vec2(4.0f), 0.0f, btnBg);
    }

    // 2. ВКЛАДКИ ПАЛИТРЫ (прямо над нижней панелью)
    for (size_t i = 0; i < m_tabButtons.size(); ++i) {
        const auto& btn = m_tabButtons[i];
        bool isSelected = ((i == 0 && m_currentCategory == PaletteCategory::Tiles) ||
                           (i == 1 && m_currentCategory == PaletteCategory::SpecialObjects) ||
                           (i == 2 && m_currentCategory == PaletteCategory::Particles) ||
                           (i == 3 && m_currentCategory == PaletteCategory::Decorations));

        glm::vec3 tabBg = isSelected ? glm::vec3(0.20f, 0.24f, 0.32f) : glm::vec3(0.11f, 0.12f, 0.16f);
        glm::vec3 tabBorder = isSelected ? glm::vec3(0.40f, 0.75f, 1.0f) : glm::vec3(0.25f, 0.27f, 0.32f);

        renderer->drawSprite(whiteTexture, btn.pos, btn.size, 0.0f, tabBorder);
        renderer->drawSprite(whiteTexture, btn.pos + glm::vec2(2.0f), btn.size - glm::vec2(4.0f), 0.0f, tabBg);
    }

    // 3. НИЖНЯЯ ПАНЕЛЬ ТУЛБАРА (Dock)
    float dockY = screenH - m_dockH;
    renderer->drawSprite(whiteTexture, glm::vec2(0.0f, dockY), glm::vec2(screenW, m_dockH), 0.0f, glm::vec3(0.07f, 0.08f, 0.10f));
    renderer->drawSprite(whiteTexture, glm::vec2(0.0f, dockY), glm::vec2(screenW, 2.0f), 0.0f, glm::vec3(0.25f, 0.27f, 0.32f));

    // Кнопки нижнего тулбара
    for (const auto& btn : m_bottomButtons) {
        glm::vec3 btnBg = glm::vec3(0.18f, 0.20f, 0.25f);
        glm::vec3 borderColor = glm::vec3(0.35f, 0.38f, 0.45f);

        if (!btn.isAction && btn.brush == ctx.currentBrush) {
            btnBg = glm::vec3(0.25f, 0.35f, 0.50f);
            borderColor = glm::vec3(1.0f, 0.85f, 0.2f); // Золотая окантовка выбранной кисти
            if (btn.brush == EditorBrush::Eraser) {
                btnBg = glm::vec3(0.45f, 0.18f, 0.18f);
                borderColor = glm::vec3(1.0f, 0.35f, 0.35f);
            } else if (btn.brush == EditorBrush::NoBuild) {
                btnBg = glm::vec3(0.50f, 0.15f, 0.15f);
                borderColor = glm::vec3(1.0f, 0.35f, 0.35f);
            } else if (btn.brush == EditorBrush::Chasm) {
                btnBg = glm::vec3(0.12f, 0.12f, 0.18f);
                borderColor = glm::vec3(0.70f, 0.45f, 0.95f);
            } else if (btn.brush == EditorBrush::Rail) {
                btnBg = glm::vec3(0.24f, 0.25f, 0.30f);
                borderColor = glm::vec3(1.0f, 0.85f, 0.2f);
            } else if (btn.brush == EditorBrush::RailStart) {
                btnBg = glm::vec3(0.38f, 0.26f, 0.12f);
                borderColor = glm::vec3(1.0f, 0.75f, 0.2f);
            } else if (btn.brush == EditorBrush::RailEnd) {
                btnBg = glm::vec3(0.40f, 0.15f, 0.15f);
                borderColor = glm::vec3(1.0f, 0.40f, 0.40f);
            } else if (btn.brush == EditorBrush::EmitterSteamJet) {
                btnBg = glm::vec3(0.20f, 0.32f, 0.40f);
                borderColor = glm::vec3(0.60f, 0.90f, 1.0f);
            } else if (btn.brush == EditorBrush::EmitterWaterDrip) {
                btnBg = glm::vec3(0.12f, 0.25f, 0.42f);
                borderColor = glm::vec3(0.35f, 0.70f, 1.0f);
            } else if (btn.brush == EditorBrush::EmitterSparks) {
                btnBg = glm::vec3(0.38f, 0.30f, 0.12f);
                borderColor = glm::vec3(1.0f, 0.85f, 0.25f);
            } else if (btn.brush == EditorBrush::EmitterSmoke) {
                btnBg = glm::vec3(0.25f, 0.25f, 0.28f);
                borderColor = glm::vec3(0.85f, 0.85f, 0.90f);
            } else if (btn.brush == EditorBrush::EmitterFog) {
                btnBg = glm::vec3(0.22f, 0.28f, 0.26f);
                borderColor = glm::vec3(0.65f, 0.85f, 0.75f);
            } else if (btn.brush == EditorBrush::DecorBush) {
                btnBg = glm::vec3(0.18f, 0.42f, 0.20f);
                borderColor = glm::vec3(0.35f, 0.85f, 0.40f);
            } else if (btn.brush == EditorBrush::DecorGrass) {
                btnBg = glm::vec3(0.22f, 0.45f, 0.18f);
                borderColor = glm::vec3(0.45f, 0.90f, 0.35f);
            } else if (btn.brush == EditorBrush::DecorGrassField) {
                btnBg = glm::vec3(0.20f, 0.48f, 0.22f);
                borderColor = glm::vec3(0.40f, 0.95f, 0.40f);
            } else if (btn.brush == EditorBrush::DecorFlower) {
                btnBg = glm::vec3(0.45f, 0.20f, 0.28f);
                borderColor = glm::vec3(0.95f, 0.45f, 0.65f);
            } else if (btn.brush == EditorBrush::DecorStone) {
                btnBg = glm::vec3(0.26f, 0.28f, 0.32f);
                borderColor = glm::vec3(0.65f, 0.70f, 0.78f);
            } else if (btn.brush == EditorBrush::DecorHelmet) {
                btnBg = glm::vec3(0.45f, 0.36f, 0.10f);
                borderColor = glm::vec3(1.0f, 0.85f, 0.20f);
            } else if (btn.brush == EditorBrush::DecorPickaxe) {
                btnBg = glm::vec3(0.32f, 0.26f, 0.20f);
                borderColor = glm::vec3(0.85f, 0.68f, 0.45f);
            } else if (btn.brush == EditorBrush::DecorPuddle) {
                btnBg = glm::vec3(0.12f, 0.26f, 0.38f);
                borderColor = glm::vec3(0.35f, 0.75f, 1.0f);
            } else if (btn.brush == EditorBrush::DecorCrack) {
                btnBg = glm::vec3(0.20f, 0.18f, 0.22f);
                borderColor = glm::vec3(0.55f, 0.50f, 0.60f);
            } else if (btn.brush == EditorBrush::DecorFog) {
                btnBg = glm::vec3(0.24f, 0.28f, 0.28f);
                borderColor = glm::vec3(0.70f, 0.82f, 0.80f);
            } else if (btn.brush == EditorBrush::DecorWoodenCrate) {
                btnBg = glm::vec3(0.28f, 0.22f, 0.15f);
                borderColor = glm::vec3(0.70f, 0.55f, 0.38f);
            } else if (btn.brush == EditorBrush::DecorBrokenCart) {
                btnBg = glm::vec3(0.20f, 0.22f, 0.25f);
                borderColor = glm::vec3(0.45f, 0.70f, 0.85f);
            } else if (btn.brush == EditorBrush::DecorScrapDebris) {
                btnBg = glm::vec3(0.22f, 0.20f, 0.18f);
                borderColor = glm::vec3(0.60f, 0.55f, 0.50f);
            } else if (btn.brush == EditorBrush::DecorDeadTree) {
                btnBg = glm::vec3(0.25f, 0.20f, 0.16f);
                borderColor = glm::vec3(0.65f, 0.52f, 0.40f);
            } else if (btn.brush == EditorBrush::DecorDryGrass) {
                btnBg = glm::vec3(0.28f, 0.24f, 0.12f);
                borderColor = glm::vec3(0.75f, 0.65f, 0.30f);
            } else if (btn.brush == EditorBrush::DecorOreDebris) {
                btnBg = glm::vec3(0.26f, 0.14f, 0.14f);
                borderColor = glm::vec3(0.85f, 0.25f, 0.25f);
            }
        } else if (!btn.isAction && btn.brush == EditorBrush::Eraser) {
            btnBg = glm::vec3(0.28f, 0.15f, 0.15f);
            borderColor = glm::vec3(0.6f, 0.25f, 0.25f);
        } else if (!btn.isAction && btn.brush == EditorBrush::NoBuild) {
            btnBg = glm::vec3(0.30f, 0.12f, 0.12f);
            borderColor = glm::vec3(0.65f, 0.25f, 0.25f);
        } else if (!btn.isAction && btn.brush == EditorBrush::Chasm) {
            btnBg = glm::vec3(0.10f, 0.10f, 0.14f);
            borderColor = glm::vec3(0.35f, 0.30f, 0.45f);
        } else if (!btn.isAction && btn.brush == EditorBrush::Rail) {
            btnBg = glm::vec3(0.16f, 0.16f, 0.20f);
            borderColor = glm::vec3(0.35f, 0.38f, 0.44f);
        } else if (!btn.isAction && btn.brush == EditorBrush::RailStart) {
            btnBg = glm::vec3(0.22f, 0.16f, 0.10f);
            borderColor = glm::vec3(0.55f, 0.40f, 0.20f);
        } else if (!btn.isAction && btn.brush == EditorBrush::RailEnd) {
            btnBg = glm::vec3(0.24f, 0.12f, 0.12f);
            borderColor = glm::vec3(0.55f, 0.25f, 0.25f);
        } else if (btn.isAction) {
            if (btn.actionId == 2) { // Test
                btnBg = glm::vec3(0.15f, 0.35f, 0.20f);
                borderColor = glm::vec3(0.25f, 0.75f, 0.35f);
            } else if (btn.actionId == 1) { // Save
                btnBg = glm::vec3(0.15f, 0.25f, 0.38f);
                borderColor = glm::vec3(0.3f, 0.6f, 0.9f);
            } else if (btn.actionId == 4) { // Exit
                btnBg = glm::vec3(0.35f, 0.15f, 0.15f);
                borderColor = glm::vec3(0.7f, 0.3f, 0.3f);
            } else if (btn.actionId == 6) { // ID Display
                btnBg = getIdColor(ctx.selectedId) * 0.3f;
                borderColor = getIdColor(ctx.selectedId);
            } else if (btn.actionId == 13) { // Waves
                if (ctx.isWaveModalOpen) {
                    btnBg = glm::vec3(0.20f, 0.40f, 0.65f);
                    borderColor = glm::vec3(0.35f, 0.85f, 1.0f);
                } else {
                    btnBg = glm::vec3(0.18f, 0.28f, 0.45f);
                    borderColor = glm::vec3(0.30f, 0.50f, 0.80f);
                }
            } else if (btn.actionId == 18) { // Minecart Settings
                if (ctx.isMinecartModalOpen) {
                    btnBg = glm::vec3(0.45f, 0.32f, 0.10f);
                    borderColor = glm::vec3(1.0f, 0.80f, 0.20f);
                } else {
                    btnBg = glm::vec3(0.26f, 0.20f, 0.09f);
                    borderColor = glm::vec3(0.70f, 0.55f, 0.20f);
                }
            } else if (btn.actionId == 25) { // Grid Snap
                if (ctx.decorGridSnap) {
                    btnBg = glm::vec3(0.15f, 0.38f, 0.22f);
                    borderColor = glm::vec3(0.35f, 0.95f, 0.45f);
                } else {
                    btnBg = glm::vec3(0.20f, 0.22f, 0.26f);
                    borderColor = glm::vec3(0.40f, 0.45f, 0.50f);
                }
            }
        }

        renderer->drawSprite(whiteTexture, btn.pos, btn.size, 0.0f, borderColor);
        renderer->drawSprite(whiteTexture, btn.pos + glm::vec2(2.0f), btn.size - glm::vec2(4.0f), 0.0f, btnBg);
    }

    renderer->flush();

    // 3. ТЕКСТ (Хедер, кнопки, статус)
    if (textRenderer) {
        float scale = GetUIScale(ctx.screenWidth, ctx.screenHeight);
        float titleFontScale = std::clamp(0.80f * scale, 0.60f, 0.95f);
        float titleY = (m_topBarH - titleFontScale * 28.0f) * 0.5f;
        textRenderer->RenderText("MAP EDITOR", 15.0f, titleY, titleFontScale, glm::vec3(1.0f, 0.85f, 0.2f));

        // Текст на кнопках верхнего бара
        for (const auto& btn : m_topButtons) {
            glm::vec3 textColor = (btn.actionId == 8) ? glm::vec3(0.4f, 0.9f, 1.0f) :
                                  (btn.actionId == 14) ? glm::vec3(1.0f, 0.9f, 0.35f) :
                                  (btn.actionId == 15) ? glm::vec3(0.4f, 1.0f, 0.5f) :
                                  (btn.actionId == 16) ? glm::vec3(0.7f, 0.9f, 1.0f) :
                                  (btn.actionId == 18) ? glm::vec3(0.45f, 0.95f, 0.55f) :
                                  (btn.actionId == 19) ? (ctx.cameraConfigMode ? glm::vec3(0.45f, 0.95f, 1.0f) : (ctx.cameraIsCustom ? glm::vec3(0.60f, 0.88f, 1.0f) : glm::vec3(0.75f, 0.78f, 0.82f))) :
                                  (btn.actionId == 20) ? glm::vec3(0.45f, 1.0f, 0.60f) :
                                  (btn.actionId == 21) ? glm::vec3(1.0f, 0.75f, 0.45f) :
                                  (btn.actionId == 22) ? (ctx.isHelpModalOpen ? glm::vec3(0.45f, 0.95f, 1.0f) : glm::vec3(1.0f, 0.90f, 0.40f)) :
                                  (btn.actionId == 23) ? (ctx.hudPreviewMode != HudPreviewMode::None ? glm::vec3(1.0f, 0.85f, 0.35f) : glm::vec3(0.75f, 0.78f, 0.82f)) :
                                  (btn.actionId == 24) ? (ctx.isLevelSettingsModalOpen ? glm::vec3(1.0f, 0.90f, 0.40f) : glm::vec3(0.85f, 0.92f, 1.0f)) : glm::vec3(0.9f);
            float tw = textRenderer->CalculateTextWidth(btn.label, m_topFontScale);
            float tx = btn.pos.x + (btn.size.x - tw) * 0.5f;
            float ty = btn.pos.y + (btn.size.y - m_topFontScale * 28.0f) * 0.5f + 2.0f;
            textRenderer->RenderText(btn.label, tx, ty, m_topFontScale, textColor);
        }

        // Статус / предупреждения строго между кнопками слева и размерными кнопками справа
        float rightControlsX = m_topButtons.empty() ? screenW - 260.0f : m_topButtons.back().pos.x - 170.0f;
        float statusStartX = m_topBarLeftEndX + 16.0f;
        float maxStatusWidth = (rightControlsX - 16.0f) - statusStartX;

        if (maxStatusWidth > 60.0f) {
            std::string statusText = "";
            glm::vec3 curColor = ctx.statusColor;

            if (ctx.hasInvalidSpawner) {
                statusText = "[!] SPAWNER BLOCKED (NO PATH)";
                curColor = glm::vec3(1.0f, 0.25f, 0.25f);
            } else if (ctx.missingBaseWarning) {
                statusText = "[i] Base #" + std::to_string(ctx.missingBaseId) + " missing (using nearest)";
                curColor = glm::vec3(1.0f, 0.75f, 0.2f);
            } else if (ctx.hasMinecartStartEnd) {
                if (ctx.isRailPathValid) {
                    statusText = "[+] РЕЙКИ: ОК (" + std::to_string(static_cast<int>(ctx.railPathSize)) + " кл.)";
                    curColor = glm::vec3(0.25f, 1.0f, 0.55f);
                } else {
                    statusText = "[!] РЕЙКИ: РОЗІРВАНІ";
                    curColor = glm::vec3(1.0f, 0.28f, 0.28f);
                }
            } else if (ctx.statusTimer > 0.0f && !ctx.statusMessage.empty()) {
                statusText = ctx.statusMessage;
                curColor = ctx.statusColor;
            }

            if (!statusText.empty()) {
                float sScale = std::clamp(0.48f * scale, 0.36f, 0.55f);
                float sw = textRenderer->CalculateTextWidth(statusText, sScale);
                if (sw > maxStatusWidth) {
                    sScale = std::max(0.32f, sScale * (maxStatusWidth / sw));
                }
                float statusY = (m_topBarH - sScale * 28.0f) * 0.5f + 1.0f;
                textRenderer->RenderText(statusText, statusStartX, statusY, sScale, curColor);
            }
        }

        // Текст на вкладках палитры
        for (size_t i = 0; i < m_tabButtons.size(); ++i) {
            const auto& btn = m_tabButtons[i];
            bool isSelected = ((i == 0 && m_currentCategory == PaletteCategory::Tiles) ||
                               (i == 1 && m_currentCategory == PaletteCategory::SpecialObjects) ||
                               (i == 2 && m_currentCategory == PaletteCategory::Particles) ||
                               (i == 3 && m_currentCategory == PaletteCategory::Decorations));

            glm::vec3 textColor = isSelected ? glm::vec3(0.40f, 0.90f, 1.0f) : glm::vec3(0.65f, 0.70f, 0.78f);
            float tw = textRenderer->CalculateTextWidth(btn.label, m_bottomFontScale);
            float tx = btn.pos.x + (btn.size.x - tw) * 0.5f;
            float ty = btn.pos.y + (btn.size.y - m_bottomFontScale * 28.0f) * 0.5f + 1.0f;
            textRenderer->RenderText(btn.label, tx, ty, m_bottomFontScale, textColor);
        }

        // Текст на кнопках нижнего бара
        for (const auto& btn : m_bottomButtons) {
            glm::vec3 textColor = glm::vec3(0.95f);
            if (!btn.isAction && btn.brush == ctx.currentBrush) {
                textColor = glm::vec3(1.0f, 0.9f, 0.3f);
            } else if (!btn.isAction && btn.brush == EditorBrush::RailStart) {
                textColor = glm::vec3(0.95f, 0.80f, 0.45f);
            } else if (!btn.isAction && btn.brush == EditorBrush::RailEnd) {
                textColor = glm::vec3(0.95f, 0.55f, 0.55f);
            } else if (!btn.isAction && btn.brush == EditorBrush::NoBuild) {
                textColor = glm::vec3(1.0f, 0.70f, 0.70f);
            } else if (btn.actionId == 6) {
                textColor = getIdColor(ctx.selectedId);
            } else if (btn.actionId == 13) {
                textColor = ctx.isWaveModalOpen ? glm::vec3(0.4f, 1.0f, 1.0f) : glm::vec3(0.6f, 0.85f, 1.0f);
            } else if (btn.actionId == 18) {
                textColor = ctx.isMinecartModalOpen ? glm::vec3(1.0f, 0.90f, 0.25f) : glm::vec3(0.90f, 0.72f, 0.30f);
            } else if (btn.actionId == 25) {
                textColor = ctx.decorGridSnap ? glm::vec3(0.4f, 1.0f, 0.5f) : glm::vec3(0.7f, 0.75f, 0.8f);
            } else if (btn.actionId == 26 || btn.actionId == 27 || btn.actionId == 28) {
                textColor = glm::vec3(0.40f, 0.90f, 1.0f);
            }
            float tw = textRenderer->CalculateTextWidth(btn.label, m_bottomFontScale);
            float tx = btn.pos.x + (btn.size.x - tw) * 0.5f;
            float ty = btn.pos.y + (btn.size.y - m_bottomFontScale * 28.0f) * 0.5f + 2.0f;
            textRenderer->RenderText(btn.label, tx, ty, m_bottomFontScale, textColor);
        }

        // Всплывающая подсказка (Tooltip) при наведении курсора
        std::string hoveredTooltip = "";
        glm::vec2 tooltipPos = mousePos;
        for (const auto& btn : m_topButtons) {
            if (!btn.tooltip.empty() && isPointInRect(mousePos, btn.pos, btn.size)) {
                hoveredTooltip = btn.tooltip;
                tooltipPos = glm::vec2(btn.pos.x, btn.pos.y + btn.size.y + 6.0f);
                break;
            }
        }
        if (hoveredTooltip.empty()) {
            for (const auto& btn : m_bottomButtons) {
                if (!btn.tooltip.empty() && isPointInRect(mousePos, btn.pos, btn.size)) {
                    hoveredTooltip = btn.tooltip;
                    tooltipPos = glm::vec2(btn.pos.x, btn.pos.y - 28.0f);
                    break;
                }
            }
        }

        if (!hoveredTooltip.empty() && renderer) {
            float tScale = std::clamp(0.44f * scale, 0.35f, 0.52f);
            float tTextW = textRenderer->CalculateTextWidth(hoveredTooltip, tScale);
            float padX = 8.0f;
            float padY = 4.0f;
            float boxW = tTextW + padX * 2.0f;
            float boxH = tScale * 28.0f + padY * 2.0f;

            if (tooltipPos.x + boxW > screenW - 8.0f) {
                tooltipPos.x = screenW - boxW - 8.0f;
            }
            if (tooltipPos.x < 8.0f) tooltipPos.x = 8.0f;

            // Отрисовываем фон тултипа
            renderer->drawSprite(whiteTexture, tooltipPos, glm::vec2(boxW, boxH), 0.0f, glm::vec3(0.35f, 0.65f, 0.95f));
            renderer->drawSprite(whiteTexture, tooltipPos + glm::vec2(1.0f), glm::vec2(boxW - 2.0f, boxH - 2.0f), 0.0f, glm::vec3(0.10f, 0.12f, 0.16f));
            renderer->flush();

            textRenderer->RenderText(hoveredTooltip, tooltipPos.x + padX, tooltipPos.y + padY + 1.0f, tScale, glm::vec3(0.95f, 0.98f, 1.0f));
        }
    }
}

void EditorToolbarUI::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                             float screenWidth, float screenHeight, const EditorContext& ctx) {
    auto whiteTex = ResourceManager::getWhiteTexture();
    EditorContext renderCtx = ctx;
    renderCtx.screenWidth = static_cast<int>(screenWidth);
    renderCtx.screenHeight = static_cast<int>(screenHeight);
    render(renderer, textRenderer, whiteTex, renderCtx, glm::vec2(0.0f));
}
