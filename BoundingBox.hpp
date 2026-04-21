#ifndef BOUNDINGBOX_HPP
#define BOUNDINGBOX_HPP
#include <glm/glm.hpp>
#include "Frustum.hpp"

class BoundingBox {
public:
    BoundingBox(glm::vec3 center, glm::vec3 extents);
    bool isOnFrustum(const Frustum& camFrustum, const glm::mat4& transform) const;

private:
    glm::vec3 mCenter;
    glm::vec3 mExtents;
    bool isOnOrForwardPlane(const Plane& plane) const;
};
#endif