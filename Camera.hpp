#ifndef Camera_hpp
#define Camera_hpp
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <vector>
namespace gps {
    enum MOVE_DIRECTION {MOVE_FORWARD, MOVE_BACKWARD, MOVE_RIGHT, MOVE_LEFT};
    class Camera {
    public:
        Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp);
        glm::mat4 getViewMatrix();
        glm::vec3 getPosition();
        void move(MOVE_DIRECTION direction, float speed);
        void rotate(float pitch, float yaw);
        void setPosition(glm::vec3 pos);
        void setTarget(glm::vec3 target);
        glm::vec3 getTarget();
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
        bool presentationActive = false;
        float presentationTime = 0.0f;
        std::vector<glm::vec3> waypoints;
        int currentSegment = 0;
    };    
}
#endif  