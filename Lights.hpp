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

struct PointLightData {
    glm::vec3 position;
    float constant;
    glm::vec3 ambient;
    float linear;
    glm::vec3 diffuse;
    float quadratic;
    glm::vec3 specular;
    float padding1;
};


#endif