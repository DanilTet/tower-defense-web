#pragma once
#include <glad/glad.h>
#include <string>

class ShaderProgram
{
public:
	ShaderProgram() : m_id(0) {
	}

	~ShaderProgram();

	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;

	ShaderProgram(ShaderProgram&& other) noexcept;
	ShaderProgram& operator =(ShaderProgram&& other) noexcept;

	bool loadShaders(const std::string& vertexPath, const std::string& fragmentPath);
	void use() const;
	void deleteProgram();
	unsigned int getId() const { return m_id; }

private:
	GLuint m_id;

	std::string readFile(const std::string& filePath) const;
	bool checkCompileErrors(GLuint shader, const std::string& type) const;
};