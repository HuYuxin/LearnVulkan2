#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(const float swapchainExtentWidth, const float swapchainExtentHeight) : mFovY(glm::radians(45.0f)),
                    mAspect(swapchainExtentWidth / swapchainExtentHeight),
                    mNear(0.1f),
                    mFar(100.0f),
                    mPosition(glm::vec3(-20.0f, 10.0f, -20.0f)),
                    mUp(glm::vec3(0.0f, 1.0f, 0.0f)),
                    mForward(glm::vec3(0.0f, 0.0f, 1.0f))
                 {}

glm::vec3 Camera::getCameraPosition() const {
    return mPosition;
}

glm::vec3 Camera::getCameraUp() const {
    return mUp;
}

glm::vec3 Camera::getCameraLookAtPosition() const {
    return mPosition + mForward;
}

float Camera::getFovY() const {
    return mFovY;
}

float Camera::getAspect() const {
    return mAspect;
}

float Camera::getNear() const {
    return mNear;
}

float Camera::getFar() const {
    return mFar;
}

void Camera::translate(const CameraMovement movement) {
    switch (movement) {
        case CameraMovement::FORWARD:
            mPosition = mPosition + mForward;
            break;
        case CameraMovement::BACKWARD:
            mPosition = mPosition + (-1.0f) * mForward;
            break;
        case CameraMovement::UP:
            mPosition = mPosition + mUp;
            break;
        case CameraMovement::DOWN:
            mPosition = mPosition + (-1.0f) * mUp;
            break;
        case CameraMovement::LEFT:
            mPosition = mPosition + glm::normalize(glm::cross(mUp, mForward));
            break;
        case CameraMovement::RIGHT:
            mPosition = mPosition + (-1.0f) * glm::normalize(glm::cross(mUp, mForward));
            break;
        default:
            // Do not move
            break;
    }
}

void Camera::rotateAroundCameraUpAxis(const float angle) {
    glm::vec3 cameraLeft = glm::cross(mUp, mForward);
    glm::vec3 rotationAxis = glm::cross(mForward, cameraLeft);

    // 1. Create a 3x3 rotation matrix directly
    // glm::mat3(glm::rotate(...)) extracts the top-left 3x3 sub-matrix.
    glm::mat3 rotationMatrix3 = glm::mat3(glm::rotate(glm::mat4(1.0f), 
                                                 glm::radians(angle), 
                                                 rotationAxis));

    // 2. Perform Matrix * Vector multiplication with vec3 and mat3
    mForward = rotationMatrix3 * mForward;
    mForward = glm::normalize(mForward);
}

void Camera::rotateAroundCameraLeftAxis(const float angle) {
    glm::vec3 rotationAxis = glm::cross(mUp, mForward);

    // 1. Create a 3x3 rotation matrix directly
    // glm::mat3(glm::rotate(...)) extracts the top-left 3x3 sub-matrix.
    glm::mat3 rotationMatrix3 = glm::mat3(glm::rotate(glm::mat4(1.0f), 
                                                 glm::radians(angle), 
                                                 rotationAxis));

    // 2. Perform Matrix * Vector multiplication with vec3 and mat3
    mForward = rotationMatrix3 * mForward;
    mForward = glm::normalize(mForward);
}
