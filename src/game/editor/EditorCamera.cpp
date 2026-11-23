#include "EditorCamera.h"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 EditorCamera::viewMatrix() const {
    glm::mat4 V = glm::mat4(1.0f);
    V = glm::translate(V, glm::vec3(pan, 0.0f));
    V = glm::scale(V, glm::vec3(zoom, zoom, 1.0f));
    return V;
}

