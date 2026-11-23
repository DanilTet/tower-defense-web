#pragma once

struct PlayerStats {
    int money = 50; // деньги (бюджет на 10 базовых башен)
    int baseHealth = 20; // хпшки базы
    int score = 0; // очки надо будет сделать чтобі при сметрі враги давали условніе 100 балов и в конце вівести их

    // метод для начисления денег (бонусы, ранний вызов волны, награды)
    void addMoney(int amount) {
        money += amount;
    }

    // метод для ресета всего при рестарте игрі
    void reset(int startMoney = 50, int startHealth = 20) {
        money = startMoney;
        baseHealth = startHealth;
        score = 0;
    }
};