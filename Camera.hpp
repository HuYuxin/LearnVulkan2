#ifndef CAMERA_HPP
#define CAMERA_HPP
#include <glm/glm.hpp>
#include "Frustum.hpp"

enum class CameraMovement {
    FORWARD,
    BACKWARD,
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class Camera {
public:
    Camera(const float swapchainExtentWidth, const float swapchainExtentHeight);
    glm::vec3 getCameraPosition() const;
    glm::vec3 getCameraUp() const;
    glm::vec3 getCameraLookAtPosition() const;
    float getFovY() const;
    float getAspect() const;
    float getNear() const;
    float getFar() const;
    Frustum getFrustum() const;
    void translate(const CameraMovement movement, const float movementSpeed);
    void orbitAroundObjectUpAxis(const float angle, const float x, const float z);
    void orbitAroundObjectHorizontalAxis(const float angle, const float x, const float y, const float z);

private:
    glm::vec3 mPosition;
    glm::vec3 mUp;
    glm::vec3 mForward;
    float mFovY;
    float mAspect;
    float mNear;
    float mFar;
};

#endif