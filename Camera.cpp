#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    glm::vec3 Camera::getPosition() {
        return cameraPosition;
    }

    //update the camera internal parameters following a camera move event
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

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        
        cameraFrontDirection = glm::normalize(front);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }
    
    // Direct Control
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
    
    // --- PRESENTATION MODE ---
    
    void Camera::startPresentation() {
        presentationActive = true;
        presentationTime = 0.0f;
        currentSegment = 0;
        waypoints.clear();
        
        // Define Scenic Route
        waypoints.push_back(glm::vec3(0.0f, 400.0f, 400.0f));   // 1. High Oribt View
        waypoints.push_back(glm::vec3(300.0f, 150.0f, 0.0f));   // 2. Asteroid Belt Level
        waypoints.push_back(glm::vec3(0.0f, 50.0f, -300.0f));   // 3. Low Pass behind spires
        waypoints.push_back(glm::vec3(-200.0f, 20.0f, 100.0f)); // 4. Ground Skim
        waypoints.push_back(glm::vec3(0.0f, 20.0f, 30.0f));     // 5. Approach Drone (End)
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
        
        float speed = 0.2f; // Segments per second (approx 5 sec per segment)
        presentationTime += delta * speed;
        
        // Calculate current segment index based on time
        int segIndex = (int)presentationTime;
        float t = presentationTime - segIndex; // Local t [0, 1]
        
        if (segIndex >= waypoints.size() - 1) {
            // Loop or Stop? Let's Loop for demo
            presentationTime = 0.0f;
            segIndex = 0;
            t = 0.0f;
            // Or stop:
            // stopPresentation(); return;
        }
        
        glm::vec3 p0 = waypoints[segIndex];
        glm::vec3 p1 = waypoints[segIndex + 1];
        
        // Interpolate Position
        cameraPosition = glm::mix(p0, p1, t);
        
        // Look at CENTER (0,0,0) or interpolated target?
        // Let's look at Center (0,20,0) mostly, where the drone usually is.
        glm::vec3 target = glm::vec3(0.0f, 20.0f, 0.0f);
        cameraFrontDirection = glm::normalize(target - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }

}
