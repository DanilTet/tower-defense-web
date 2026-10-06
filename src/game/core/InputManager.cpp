#include "InputManager.h"
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <cctype>

std::string InputManager::s_frameInputUtf8 = "";

void InputManager::init(GLFWwindow* window) {
    glfwSetCharCallback(window, [](GLFWwindow* win, unsigned int codepoint) {
        InputManager::onCharCallback(win, codepoint);
    });
}

void InputManager::onCharCallback(GLFWwindow* /*window*/, unsigned int codepoint) {
    // Игнорируем непечатаемые управляющие символы
    if (codepoint < 32 || (codepoint >= 127 && codepoint < 160)) {
        return;
    }
    s_frameInputUtf8 += codepointToUtf8(static_cast<char32_t>(codepoint));
}

std::string InputManager::getFrameText() {
    return s_frameInputUtf8;
}

void InputManager::clearFrameText() {
    s_frameInputUtf8.clear();
}

void InputManager::popUtf8(std::string& s) {
    if (s.empty()) return;
    // Откатываемся по продолжениям байтов UTF-8 (10xxxxxx)
    while (!s.empty() && (static_cast<unsigned char>(s.back()) & 0xC0) == 0x80) {
        s.pop_back();
    }
    // Удаляем лидирующий байт
    if (!s.empty()) {
        s.pop_back();
    }
}

std::string InputManager::codepointToUtf8(char32_t cp) {
    std::string out;
    if (cp <= 0x7F) {
        out += static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        out += static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        out += static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0x10FFFF) {
        out += static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return out;
}

std::string InputManager::transliterateToAscii(const std::string& utf8Text) {
    static const std::unordered_map<char32_t, std::string> s_cyrillicMap = {
        { 0x0410, "a" }, { 0x0430, "a" },
        { 0x0411, "b" }, { 0x0431, "b" },
        { 0x0412, "v" }, { 0x0432, "v" },
        { 0x0413, "g" }, { 0x0433, "g" },
        { 0x0490, "g" }, { 0x0491, "g" }, // Ґ, ґ
        { 0x0414, "d" }, { 0x0434, "d" },
        { 0x0415, "e" }, { 0x0435, "e" },
        { 0x0401, "yo" }, { 0x0451, "yo" }, // Ё, ё
        { 0x0404, "ye" }, { 0x0454, "ye" }, // Є, є
        { 0x0416, "zh" }, { 0x0436, "zh" },
        { 0x0417, "z" }, { 0x0437, "z" },
        { 0x0418, "y" }, { 0x0438, "y" },
        { 0x0406, "i" }, { 0x0456, "i" }, // І, і
        { 0x0407, "yi" }, { 0x0457, "yi" }, // Ї, ї
        { 0x0419, "y" }, { 0x0439, "y" },
        { 0x041A, "k" }, { 0x043A, "k" },
        { 0x041B, "l" }, { 0x043B, "l" },
        { 0x041C, "m" }, { 0x043C, "m" },
        { 0x041D, "n" }, { 0x043D, "n" },
        { 0x041E, "o" }, { 0x043E, "o" },
        { 0x041F, "p" }, { 0x043F, "p" },
        { 0x0420, "r" }, { 0x0440, "r" },
        { 0x0421, "s" }, { 0x0441, "s" },
        { 0x0422, "t" }, { 0x0442, "t" },
        { 0x0423, "u" }, { 0x0443, "u" },
        { 0x0424, "f" }, { 0x0444, "f" },
        { 0x0425, "kh" }, { 0x0445, "kh" },
        { 0x0426, "ts" }, { 0x0446, "ts" },
        { 0x0427, "ch" }, { 0x0447, "ch" },
        { 0x0428, "sh" }, { 0x0448, "sh" },
        { 0x0429, "shch" }, { 0x0449, "shch" },
        { 0x042A, "" }, { 0x044A, "" }, // Ъ, ъ
        { 0x042B, "y" }, { 0x044B, "y" },
        { 0x042C, "" }, { 0x044C, "" }, // Ь, ь
        { 0x042D, "e" }, { 0x044D, "e" },
        { 0x042E, "yu" }, { 0x044E, "yu" },
        { 0x042F, "ya" }, { 0x044F, "ya" }
    };

    std::string result;
    for (size_t i = 0; i < utf8Text.length(); ) {
        char32_t codepoint = 0;
        unsigned char c = utf8Text[i];
        if (c < 0x80) { codepoint = c; i++; }
        else if ((c & 0xE0) == 0xC0 && i + 1 < utf8Text.length()) { codepoint = ((c & 0x1F) << 6) | (utf8Text[i + 1] & 0x3F); i += 2; }
        else if ((c & 0xF0) == 0xE0 && i + 2 < utf8Text.length()) { codepoint = ((c & 0x0F) << 12) | ((utf8Text[i + 1] & 0x3F) << 6) | (utf8Text[i + 2] & 0x3F); i += 3; }
        else { i++; continue; }

        auto it = s_cyrillicMap.find(codepoint);
        if (it != s_cyrillicMap.end()) {
            result += it->second;
        } else if ((codepoint >= 'a' && codepoint <= 'z') || (codepoint >= '0' && codepoint <= '9')) {
            result += static_cast<char>(codepoint);
        } else if (codepoint >= 'A' && codepoint <= 'Z') {
            result += static_cast<char>(std::tolower(static_cast<char>(codepoint)));
        } else if (codepoint == ' ' || codepoint == '-' || codepoint == '_') {
            if (result.empty() || result.back() != '_') {
                result += '_';
            }
        }
    }

    while (!result.empty() && result.back() == '_') result.pop_back();
    while (!result.empty() && result.front() == '_') result.erase(result.begin());

    return result.empty() ? "custom_map" : result;
}

