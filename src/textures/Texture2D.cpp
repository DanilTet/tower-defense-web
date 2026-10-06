#include "Texture2D.h"
#include <stb/stb_image.h>
#include <iostream>

Texture2D::Texture2D() : m_id(0), m_width(0), m_height(0) {}

Texture2D::~Texture2D() {
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
    }
}

Texture2D::Texture2D(Texture2D&& other) noexcept {
    m_id = other.m_id;
    m_width = other.m_width;
    m_height = other.m_height;
    other.m_id = 0;
    other.m_width = 0;
    other.m_height = 0;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        if (m_id != 0) {
            glDeleteTextures(1, &m_id);
        }

        m_id = other.m_id;
        m_width = other.m_width;
        m_height = other.m_height;

        other.m_id = 0;
        other.m_width = 0;
        other.m_height = 0;
    }
    return *this;
}

bool Texture2D::load(const std::string& path) {
    stbi_set_flip_vertically_on_load(false);

    int channels;
    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &channels, 0);

    if (!data) {
        std::cerr << "Failed to load texture at path: " << path << std::endl;
        return false;
    }

    GLenum format = GL_RGB;
    if (channels == 4) {
        format = GL_RGBA;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        m_width,
        m_height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

bool Texture2D::loadFromMemory(int width, int height, const unsigned char* data, int channels) {
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
    m_width = width;
    m_height = height;

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void Texture2D::bind(unsigned int unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture2D::unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}