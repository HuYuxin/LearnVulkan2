#include "BoundingBox.hpp"

BoundingBox::BoundingBox(glm::vec3 center, glm::vec3 extents):mCenter(center), mExtents(extents){}

bool BoundingBox::isOnFrustum(const Frustum& camFrustum, const glm::mat4& transform) const
{
    const glm::vec3 globalCenter{ transform * glm::vec4(mCenter, 1.f) };

    // Scaled orientation
    const glm::vec3 right = transform * glm::vec4(mExtents.x, 0.f, 0.f, 0.f);
    const glm::vec3 up = transform * glm::vec4(0.f, mExtents.y, 0.f, 0.f);
    const glm::vec3 forward = transform * glm::vec4(0.f, 0.f, mExtents.z, 0.f);

    const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
        std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
        std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));

    const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
        std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
        std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));

    const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
        std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
        std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));

    // We not need to divise scale because it's based on the half extention of the AABB
    const BoundingBox globalAABB(globalCenter, glm::vec3(newIi, newIj, newIk));

    return (globalAABB.isOnOrForwardPlane(camFrustum.mLeftPlane) &&
        globalAABB.isOnOrForwardPlane(camFrustum.mRightPlane) &&
        globalAABB.isOnOrForwardPlane(camFrustum.mTopPlane) &&
        globalAABB.isOnOrForwardPlane(camFrustum.mBottomPlane) &&
        globalAABB.isOnOrForwardPlane(camFrustum.mNearPlane) &&
        globalAABB.isOnOrForwardPlane(camFrustum.mFarPlane));
}

bool BoundingBox::isOnOrForwardPlane(const Plane& plane) const {
    const float r = mExtents.x * std::abs(plane.normal.x) +
            mExtents.y * std::abs(plane.normal.y) + mExtents.z * std::abs(plane.normal.z);

    return -r <= plane.getSignedDistanceToPlane(mCenter);
}