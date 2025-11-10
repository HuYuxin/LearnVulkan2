#ifndef CAMERA_HPP
#define CAMERA_HPP
#include <glm/glm.hpp>

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
    void translate(const glm::vec3 direction);
    void rotateAroundCameraUpAxis(const float angle);
    void rotateAroundCameraLeftAxis(const float angle);

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