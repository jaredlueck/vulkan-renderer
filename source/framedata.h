#pragma once
#include <glm/glm.hpp>

struct FrameData {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 lightDir;
    glm::vec4 cameraPos;
};