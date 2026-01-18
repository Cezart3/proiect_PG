#include "Camera.hpp"
namespace gps {
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }
    glm::vec3 Camera::getPosition() {
        return cameraPosition;
    }
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        switch (direction) {
            case MOVE_FORWARD:
                cameraPosition += cameraFrontDirection * speed;
                break;
            case MOVE_BACKWARD:
                cameraPosition -= cameraFrontDirection * speed;
                break;
            case MOVE_RIGHT:
                cameraPosition += cameraRightDirection * speed;
                break;
            case MOVE_LEFT:
                cameraPosition -= cameraRightDirection * speed;
                break;
        }
    }
    void Camera::rotate(float pitch, float yaw) {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFrontDirection = glm::normalize(front);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }
    void Camera::setPosition(glm::vec3 pos) {
        cameraPosition = pos;
    }
    void Camera::setTarget(glm::vec3 target) {
        cameraTarget = target;
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }
    glm::vec3 Camera::getTarget() {
        return cameraTarget;
    }
    void Camera::startPresentation() {
        presentationActive = true;
        presentationTime = 0.0f;
        currentSegment = 0;
        waypoints.clear();
        waypoints.push_back(glm::vec3(0.0f, 400.0f, 400.0f));   
        waypoints.push_back(glm::vec3(300.0f, 150.0f, 0.0f));   
        waypoints.push_back(glm::vec3(0.0f, 50.0f, -300.0f));   
        waypoints.push_back(glm::vec3(-200.0f, 20.0f, 100.0f)); 
        waypoints.push_back(glm::vec3(0.0f, 20.0f, 30.0f));     
    }
    void Camera::stopPresentation() {
        presentationActive = false;
    }
    bool Camera::isPresentationActive() {
        return presentationActive;
    }
    void Camera::updatePresentation(float delta) {
        if (!presentationActive) return;
        if (waypoints.empty()) return;
        float speed = 0.2f; 
        presentationTime += delta * speed;
        int segIndex = (int)presentationTime;
        float t = presentationTime - segIndex; 
        if (segIndex >= waypoints.size() - 1) {
            presentationTime = 0.0f;
            segIndex = 0;
            t = 0.0f;
        }
        glm::vec3 p0 = waypoints[segIndex];
        glm::vec3 p1 = waypoints[segIndex + 1];
        cameraPosition = glm::mix(p0, p1, t);
        glm::vec3 target = glm::vec3(0.0f, 20.0f, 0.0f);
        cameraFrontDirection = glm::normalize(target - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }
}