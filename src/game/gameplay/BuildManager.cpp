#include "BuildManager.h"
#include "world/GameWorld.h"
#include "audio/AudioManager.h"
#include <iostream>
#include "entities/Tower.h"
#include "entities/Enemy.h"
#include "world/Grid.h"
#include "world/Pathfinder.h"
#include "gameplay/PlayerStats.h"
#include "gameplay/EntityManager.h"
#include "../core/ConfigManager.h"
#include "../core/EventBus.h"
#include "../world/PathService.h"

void BuildManager::tryBuildOrUpgrade(
    glm::vec2 mousePos,
    const std::string& selectedType,
    GameWorld& world,
    float placementAngle) {

    glm::ivec2 clickedCell = world.grid->pixelToGrid(mousePos);

    // если клик поза поля
    if (clickedCell.x < 0 || clickedCell.x >= world.grid->getWidth() ||
        clickedCell.y < 0 || clickedCell.y >= world.grid->getHeight()) {
        return;
    }

    // если в руке ниче нету то просто клик по карте
    if (selectedType.empty()) {
        return;
    }

    int currentCost = ConfigManager::getTowerStats(selectedType).cost;

    if (world.playerStats.money >= currentCost && world.grid->canBuildAt(clickedCell.x, clickedCell.y)) {

        std::vector<std::vector<glm::ivec2>> newPaths;
        if (!PathService::isPlacementValid(*world.grid, *world.pathfinder, world.spawners, world.bases, clickedCell.x, clickedCell.y, newPaths, &world.paths, &world.entityManager->getEnemies())) {
            std::cout << "Path Blocked! Cannot build here." << std::endl;
            return;
        }

        world.playerStats.money -= currentCost;

        // Помечаем клетку как занятую башней на сетке
        world.grid->setCellType(clickedCell.x, clickedCell.y, CellType::Tower);

        // спавнить башню через entityManager с заданным направлением
        auto newTower = std::make_unique<Tower>(clickedCell.x, clickedCell.y, selectedType, placementAngle);
        world.entityManager->addTower(std::move(newTower));

        // звук постройки
        TowerStats towerstats = Tower::getStatsfromTowerType(selectedType);
        Event e;
        e.type = EventType::TowerBuilt;
        e.textData = towerstats.buildSound;
        EventBus::publish(e);

        // обновляем все маршруты
        world.paths = newPaths;
        world.levelPath = world.paths[0];

        // даем врагам новый путь (только тем, чей маршрут пересекает новую башню)
        world.notifyEnemiesPathChanged(clickedCell);
    }
}

void BuildManager::sellTower(
    Tower* tower,
    GameWorld& world) {

    if (!tower) return;

    int tx = tower->getGridX();
    int ty = tower->getGridY();

    world.playerStats.money += 50;

    // освобождаем клетку на сетке (возвращаем исходный тип террейна)
    world.grid->setCellType(tx, ty, world.grid->getOriginalCellType(tx, ty));

    // уничтожаем объект башни
    world.entityManager->removeTower(tx, ty);

    // пересчитываем пути и оповещаем врагов
    world.recalculateAllPaths();
    world.notifyEnemiesPathChanged();
}