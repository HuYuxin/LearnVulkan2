#ifndef LIGHTS_HPP
#define LIGHTS_HPP
#include <glm/glm.hpp>
struct DirectionalLight {
    glm::vec3 direction;
    float padding1;
    glm::vec3 ambient;
    float padding2;
    glm::vec3 diffuse;
    float padding3;
    glm::vec3 specular;
    float padding4;
};


#endif