#pragma once
#include <glad/glad.h>
#include <string>

class Texture2D {
public:
	Texture2D();
	~Texture2D();

	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;

	Texture2D(Texture2D&& other) noexcept;
	Texture2D& operator=(Texture2D&& other) noexcept;

	bool load(const std::string& path);
	bool loadFromMemory(int width, int height, const unsigned char* data, int channels = 4);

	void bind(unsigned int unit = 0) const;
	void unbind() const;

	unsigned int getId() const { return m_id; }
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }

private:
	unsigned int m_id = 0;
	int m_width = 0;
	int m_height = 0;
};