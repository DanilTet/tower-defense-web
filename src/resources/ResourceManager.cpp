#include "ResourceManager.h"
#include <iostream>

std::map<std::string, ShaderProgram> ResourceManager::m_shaders;
std::map<std::string, Texture2D> ResourceManager::m_textures;

ShaderProgram* ResourceManager::loadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
	auto it = m_shaders.find(name);
	if (it != m_shaders.end()) {
		return &it->second;
	}

	auto [insertedIt, success] = m_shaders.emplace(name, ShaderProgram());

	if (!insertedIt->second.loadShaders(vertexPath, fragmentPath)) {
		std::cerr << "ResourceManager: Failed to load shader " << name << std::endl;
		m_shaders.erase(insertedIt);
		return nullptr;
	}
	
	return &insertedIt->second;
}

Texture2D* ResourceManager::loadTexture(const std::string& name, const std::string& path) {
	auto it = m_textures.find(name);
	if (it != m_textures.end()) {
		return &it->second;
	}

	auto [insertedIt, success] = m_textures.emplace(name, Texture2D());

	if (!insertedIt->second.load(path)) {
		std::cerr << "ResourceManager: Failed to load texture " << name << std::endl;
		m_textures.erase(insertedIt);
		return nullptr;
	}

	return &insertedIt->second;
}

ShaderProgram* ResourceManager::getShader(const std::string& name) {
	auto it = m_shaders.find(name);
	if (it != m_shaders.end()) {
		return &it->second;
	}
	std::cerr << "ResourceManager: Shader \"" << name << "\" not found!" << std::endl;
	return nullptr;
}

Texture2D* ResourceManager::getTexture(const std::string& name) {
	auto it = m_textures.find(name);
	if (it != m_textures.end()) {
		return &it->second;
	}
	std::cerr << "ResourceManager: Texture \"" << name << "\" not found!" << std::endl;
	return nullptr;
}

Texture2D* ResourceManager::getWhiteTexture() {
	auto it = m_textures.find("whitePixel");
	if (it != m_textures.end()) {
		return &it->second;
	}
	auto [insertedIt, success] = m_textures.emplace("whitePixel", Texture2D());
	unsigned char white[4] = { 255, 255, 255, 255 };
	insertedIt->second.loadFromMemory(1, 1, white, 4);
	return &insertedIt->second;
}

void ResourceManager::clear() {
	m_shaders.clear();
	m_textures.clear();
}