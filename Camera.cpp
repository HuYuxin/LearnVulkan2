#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(const float swapchainExtentWidth, const float swapchainExtentHeight) : mFovY(glm::radians(45.0f)),
                    mAspect(swapchainExtentWidth / swapchainExtentHeight),
                    mNear(0.01f),
                    mFar(10.0f),
                    mPosition(glm::vec3(0.0f, 0.03f, -0.15f)),
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

void Camera::translate(const CameraMovement movement, const float movementSpeed) {
    switch (movement) {
        case CameraMovement::FORWARD:
            mPosition = mPosition + mForward * movementSpeed;
            break;
        case CameraMovement::BACKWARD:
            mPosition = mPosition + (-1.0f) * mForward * movementSpeed;
            break;
        case CameraMovement::UP:
            mPosition = mPosition + mUp * movementSpeed;
            break;
        case CameraMovement::DOWN:
            mPosition = mPosition + (-1.0f) * mUp * movementSpeed;
            break;
        case CameraMovement::LEFT:
            mPosition = mPosition + glm::normalize(glm::cross(mUp, mForward)) * movementSpeed;
            break;
        case CameraMovement::RIGHT:
            mPosition = mPosition + (-1.0f) * glm::normalize(glm::cross(mUp, mForward)) * movementSpeed;
            break;
        default:
            // Do not move
            break;
    }
}

void Camera::orbitAroundObjectUpAxis(const float angle, const float x, const float z) {
    // Pivot point: the object's xz location, at the camera's y height
    glm::vec3 pivot(x, mPosition.y, z);

    // Rotation axis: world up (0, 1, 0)
    glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);

    // 1. Translate camera position relative to pivot
    glm::vec3 relativePos = mPosition - pivot;

    // 2. Rotate the relative position around the Y-axis
    glm::mat3 rotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0f),
                                                 glm::radians(angle),
                                                 rotationAxis));
    relativePos = rotationMatrix * relativePos;

    // 3. Translate back
    mPosition = pivot + relativePos;

    // 4. Rotate the forward vector so the camera keeps looking toward the pivot
    mForward = rotationMatrix * mForward;
    mForward = glm::normalize(mForward);
}

void Camera::orbitAroundObjectHorizontalAxis(const float angle, float x, float y, float z) {
    // Pivot point
    glm::vec3 pivot(x, y, z);

    // Rotation axis
    glm::vec3 rotationAxis = glm::cross(mUp, mForward);

    // 1. Translate camera position relative to pivot
    glm::vec3 relativePos = mPosition - pivot;

    // 2. Rotate the relative position
    glm::mat3 rotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0f),
                                                 glm::radians(angle), 
                                                 rotationAxis));
    relativePos = rotationMatrix * relativePos;

    // 3. Translate back
    mPosition = pivot + relativePos;

    // 4. Rotate the forward vector so the camera keeps looking toward the pivot
    mForward = rotationMatrix * mForward;
    mForward = glm::normalize(mForward);
}

Frustum Camera::getFrustum() const {
    Frustum frustum = {};
    float tanHalfFov = tan(mFovY / 2.0f);
    float farHalfHeight = mFar * tanHalfFov;
    float farHalfWidth = farHalfHeight * mAspect;
    glm::vec3 frontFar = mFar * mForward;
    glm::vec3 cameraRight = glm::normalize(glm::cross(mUp, mForward));

    frustum.mNearPlane = {mForward, mPosition + mForward * mNear};
    frustum.mFarPlane = {-mForward, mPosition + mForward * mFar};
    frustum.mLeftPlane = {glm::cross(mUp, (frontFar - cameraRight * farHalfWidth)), mPosition};
    frustum.mRightPlane = {glm::cross((frontFar + cameraRight * farHalfWidth), mUp),  mPosition};
    frustum.mTopPlane = {glm::cross(cameraRight, (frontFar + mUp * farHalfHeight)), mPosition};
    frustum.mBottomPlane = {glm::cross((frontFar - mUp * farHalfHeight), cameraRight), mPosition};
    return frustum;
}
