#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP
#include <glm/glm.hpp>

struct Plane {
    glm::vec3 normal;
    // any points on the plane
    glm::vec3 pointsOnPlane;
    float getSignedDistanceToPlane(const glm::vec3& point) const;
};

struct Frustum {
    Plane mNearPlane;
    Plane mFarPlane;
    Plane mLeftPlane;
    Plane mRightPlane;
    Plane mTopPlane;
    Plane mBottomPlane;
};
#endif