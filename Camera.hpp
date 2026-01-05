#ifndef Camera_hpp
#define Camera_hpp

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <vector>

namespace gps {
    
    enum MOVE_DIRECTION {MOVE_FORWARD, MOVE_BACKWARD, MOVE_RIGHT, MOVE_LEFT};
    
    class Camera {

    public:
        //Camera constructor
        Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp);
        //return the view matrix, using the glm::lookAt() function
        glm::mat4 getViewMatrix();
        
        //return the camera Position
        glm::vec3 getPosition();
        //update the camera internal parameters following a camera move event
        void move(MOVE_DIRECTION direction, float speed);
        //update the camera internal parameters following a camera rotate event
        //yaw - camera rotation around the y axis
        //pitch - camera rotation around the x axis
        void rotate(float pitch, float yaw);
        
        // Direct Control
        void setPosition(glm::vec3 pos);
        void setTarget(glm::vec3 target);
        glm::vec3 getTarget();
        
        // Presentation Mode
        void startPresentation();
        void stopPresentation();
        void updatePresentation(float delta);
        bool isPresentationActive();

    private:
        glm::vec3 cameraPosition;
        glm::vec3 cameraTarget;
        glm::vec3 cameraFrontDirection;
        glm::vec3 cameraRightDirection;
        glm::vec3 cameraUpDirection;
        
        // Presentation State
        bool presentationActive = false;
        float presentationTime = 0.0f;
        std::vector<glm::vec3> waypoints;
        int currentSegment = 0;
    };    
}

#endif /* Camera_hpp */
