#include "TextRenderer.h"
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/gtc/matrix_transform.hpp>

TextRenderer::TextRenderer(std::shared_ptr<ShaderProgram> shader, int windowWidth, int windowHeight) {
    TextShader = shader;

    // настройка стартовой матрицы
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f);
    TextShader->use();
    glUniformMatrix4fv(glGetUniformLocation(TextShader->getId(), "projection"), 1, GL_FALSE, &projection[0][0]);

    // Создаем VAO и VBO для квадратного полигона, на котором будет рисоваться буква
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // 2D координаты + 2D текстурные координаты = 4 float на вершину. 6 вершин = прямоугольник
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

TextRenderer::~TextRenderer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void TextRenderer::updateProjection(const glm::mat4& projection) {
    TextShader->use();
    glUniformMatrix4fv(glGetUniformLocation(TextShader->getId(), "projection"), 1, GL_FALSE, &projection[0][0]);
}

bool TextRenderer::Load(const std::string& fontPath, unsigned int fontSize) {
    // очищаем старые символы, если грузим новый шрифт
    Characters.clear();

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font: " << fontPath << std::endl;
        return false;
    }

    // Задаем размер шрифта (0 - автоширина)
    FT_Set_Pixel_Sizes(face, 0, fontSize);

    // Отключаем ограничение выравнивания байтов в OpenGL
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Функция для загрузки диапазона символов
    auto loadRange = [&](char32_t start, char32_t end) {
        for (char32_t c = start; c <= end; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cerr << "ERROR::FREETYPE: Failed to load Glyph: " << c << std::endl;
                continue;
            }
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
#ifdef __EMSCRIPTEN__
            glTexImage2D(
                GL_TEXTURE_2D, 0, GL_R8,
                face->glyph->bitmap.width, face->glyph->bitmap.rows,
                0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer
            );
#else
            glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RED,
                face->glyph->bitmap.width, face->glyph->bitmap.rows,
                0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer
            );
#endif
            // Настройки фильтрации текстуры
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char32_t, Character>(c, character));
        }
        };

    // Загружаем Английский алфавит и символы (ASCII)

    loadRange(32, 127);
    // Дополнительные латинские символы (кавычки « », умляуты и т.д.)
    loadRange(160, 255);
    // Полный блок Кириллицы (U+0400..U+04FF): русский, украинский (включая Ґ, ґ, Є, є, І, і, Ї, ї), белорусский
    loadRange(1024, 1279);
    // Символы пунктуации (тире, кавычки-лапки, апострофы ’)
    loadRange(8210, 8230);
    // Знак номера № (U+2116)
    loadRange(8470, 8470);

    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return true;
}

void TextRenderer::RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    TextShader->use();
    glUniform3f(glGetUniformLocation(TextShader->getId(), "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    for (size_t i = 0; i < text.length(); ) {
        char32_t codepoint = 0;
        unsigned char c = text[i];
        if (c < 0x80) { codepoint = c; i++; }
        else if ((c & 0xE0) == 0xC0) { codepoint = ((c & 0x1F) << 6) | (text[i + 1] & 0x3F); i += 2; }
        else if ((c & 0xF0) == 0xE0) { codepoint = ((c & 0x0F) << 12) | ((text[i + 1] & 0x3F) << 6) | (text[i + 2] & 0x3F); i += 3; }
        else { codepoint = '?'; i++; }

        if (Characters.find(codepoint) == Characters.end()) continue; // Пропускаем неизвестные символы
        Character ch = Characters[codepoint];

        float xpos = x + ch.Bearing.x * scale;
        // Y считаем так, чтобы текст рисовался сверху-вниз (как в 2D играх), опираясь на базовую линию
        float ypos = y + (Characters['H'].Bearing.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 0.0f },
            { xpos,     ypos,       0.0f, 0.0f },

            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

float TextRenderer::CalculateTextWidth(const std::string& text, float scale) {
    float width = 0.0f;

    for (size_t i = 0; i < text.length(); ) {
        // читаем UTF-8 кодировку
        char32_t codepoint = 0;
        unsigned char c = text[i];
        if (c < 0x80) { codepoint = c; i++; }
        else if ((c & 0xE0) == 0xC0) { codepoint = ((c & 0x1F) << 6) | (text[i + 1] & 0x3F); i += 2; }
        else if ((c & 0xF0) == 0xE0) { codepoint = ((c & 0x0F) << 12) | ((text[i + 1] & 0x3F) << 6) | (text[i + 2] & 0x3F); i += 3; }
        else { codepoint = '?'; i++; }

        // если символ есть в базе то добавляем его ширину к общей
        if (Characters.find(codepoint) != Characters.end()) {
            width += (Characters[codepoint].Advance >> 6) * scale;
        }
    }

    return width;
}