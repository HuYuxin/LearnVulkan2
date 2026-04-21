#include "Frustum.hpp"

float Plane::getSignedDistanceToPlane(const glm::vec3& point) const {
    return glm::dot(point, normal) - glm::dot(pointsOnPlane, normal);
}
