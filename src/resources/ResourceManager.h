#pragma once

#include <string>
#include <map>
#include "shaders/ShaderProgram.h"
#include "textures/Texture2D.h"

class ResourceManager {
public:
    ResourceManager() = delete;
    ~ResourceManager() = delete;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    static ShaderProgram* loadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    static ShaderProgram* getShader(const std::string& name);

    static Texture2D* loadTexture(const std::string& name, const std::string& path);
    static Texture2D* getTexture(const std::string& name);
    static Texture2D* getWhiteTexture();

    static void clear();

private:
    static std::map<std::string, ShaderProgram> m_shaders;
    static std::map<std::string, Texture2D> m_textures;
};