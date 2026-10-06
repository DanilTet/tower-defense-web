#pragma once
#include <string>
#include <vector>

struct GLFWwindow;

class InputManager {
public:
    static void init(GLFWwindow* window);
    static void onCharCallback(GLFWwindow* window, unsigned int codepoint);

    // Получить введенный за кадр текст в формате UTF-8
    static std::string getFrameText();
    // Очистить буфер ввода кадра
    static void clearFrameText();

    // Безопасное удаление одного UTF-8 символа (мультибайтового)
    static void popUtf8(std::string& s);

    // Проверка, является ли строка корректным UTF-8
    static std::string codepointToUtf8(char32_t cp);

    // Транслитерация кириллицы (рус/укр) в безопасную латиницу для имен файлов на диске
    static std::string transliterateToAscii(const std::string& utf8Text);

private:
    static std::string s_frameInputUtf8;
};

