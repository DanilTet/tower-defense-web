#pragma once

struct PlayerStats {
    int money = 100000000; // деньки
    int baseHealth = 100; // хпшки
    int score = 0; // очки надо будет сделать чтобі при сметрі враги давали условніе 100 балов и в конце вівести их

    // метод для ресета всего при рестарте игрі
    void reset(int startMoney = 100000000, int startHealth = 100) {
        money = startMoney;
        baseHealth = startHealth;
        score = 0;
    }
};