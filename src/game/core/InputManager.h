#pragma once
#include <string>
#include <vector>

struct GLFWwindow;

class InputManager {
public:
    static void init(GLFWwindow* window);
    static void onCharCallback(GLFWwindow* window, unsigned int codepoint);
    static void onScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    // Получить введенный за кадр текст в формате UTF-8
    static std::string getFrameText();
    // Очистить буфер ввода кадра
    static void clearFrameText();

    // Скролл колесика мыши за кадр (dy: >0 вверх / приближение, <0 вниз / отдаление)
    static float getScrollY();
    static float getScrollX();
    static void clearScroll();

    // Безопасное удаление одного UTF-8 символа (мультибайтового)
    static void popUtf8(std::string& s);

    // Проверка, является ли строка корректным UTF-8
    static std::string codepointToUtf8(char32_t cp);

    // Транслитерация кириллицы (рус/укр) в безопасную латиницу для имен файлов на диске
    static std::string transliterateToAscii(const std::string& utf8Text);

private:
    static std::string s_frameInputUtf8;
    static float s_scrollY;
    static float s_scrollX;
};

