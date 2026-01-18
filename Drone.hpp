#ifndef Drone_hpp
#define Drone_hpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Model3D.hpp"
#include "Shader.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
namespace gps {
    class Drone {
    public:
        Drone();
        void Load(std::string modelPath);
        void Update(float delta, GLboolean pressedKeys[], class World& world);
        void Draw(gps::Shader& shader, glm::mat4 viewMatrix);
        glm::vec3 GetPosition() const;
        glm::vec3 GetForward() const;
        glm::vec3 GetUp() const; 
        void Reset(); 
        bool GetBoosting() const { return isBoosting; }
    private:
        gps::Model3D mesh;
        glm::vec3 position;
        bool isCrashed = false;
        bool isBoosting = false;
        float verticalVelocity = 0.0f; 
        float yaw;
        float pitch;
        float roll;
        float visualTilt; 
        float speed;
        float rotSpeed;
        float liftSpeed;
        float tiltSpeed;
        float rollLerpSpeed;
        float turnFactor;
        GLint modelLoc;
        GLint normalMatrixLoc;
    };
}
#endif  