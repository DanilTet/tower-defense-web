#include "ShaderProgram.h"
#include <fstream>
#include <sstream>
#include <iostream>

ShaderProgram::~ShaderProgram(){
	deleteProgram();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : m_id(other.m_id)
{
	other.m_id = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
	if (this != &other)
	{
		deleteProgram();
		m_id = other.m_id;
		other.m_id = 0;
	}
	return *this;
}

void ShaderProgram::use() const {
	if (m_id != 0) {
		glUseProgram(m_id);
	}
}

void ShaderProgram::deleteProgram() {
	if (m_id != 0) {
		glDeleteProgram(m_id);
		m_id = 0;
	}
}

std::string ShaderProgram::readFile(const std::string& filePath) const{
	std::ifstream file(filePath);
	if(!file.is_open()){
		std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << filePath << std::endl;
		return "";
	}
	std::stringstream stream;
	stream << file.rdbuf();
	return stream.str();
}

bool ShaderProgram::checkCompileErrors(GLuint shader, const std::string& type) const {
	GLint success;
	GLchar infoLog[1024];
	if (type != "PROGRAM") {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
			return false;
		}
	}
	else {
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
			return false;
		}
	}
	return true;
}

bool ShaderProgram::loadShaders(const std::string& vertexPath, const std::string& fragmentPath){
	std::string vertexCode = readFile(vertexPath);
	std::string fragmentCode = readFile(fragmentPath);

	if (vertexCode.empty() || fragmentCode.empty()) {
		return false;
	}

#ifdef __EMSCRIPTEN__
	auto adaptForWebGL = [](std::string& code) {
		size_t pos = code.find("#version");
		if (pos != std::string::npos) {
			size_t endLine = code.find('\n', pos);
			if (endLine != std::string::npos) {
				std::string firstLine = code.substr(pos, endLine - pos);
				if (firstLine.find("300 es") == std::string::npos) {
					code.replace(pos, endLine - pos, "#version 300 es\nprecision mediump float;");
				}
			}
		} else {
			code = "#version 300 es\nprecision mediump float;\n" + code;
		}
	};
	adaptForWebGL(vertexCode);
	adaptForWebGL(fragmentCode);
#endif

	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();

	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, nullptr);
	glCompileShader(vertex);
	if (!checkCompileErrors(vertex, "VERTEX")) {
		return false;
	}

	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, nullptr);
	glCompileShader(fragment);
	if (!checkCompileErrors(fragment, "FRAGMENT")) {
		return false;
	}

	m_id = glCreateProgram();
	glAttachShader(m_id, vertex);
	glAttachShader(m_id, fragment);
	glLinkProgram(m_id);
	if (!checkCompileErrors(m_id, "PROGRAM")) return false;

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return true;
}