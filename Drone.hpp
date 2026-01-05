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
        // Pass World for collision checking
        void Update(float delta, GLboolean pressedKeys[], class World& world);
        void Draw(gps::Shader& shader, glm::mat4 viewMatrix);
        
        glm::vec3 GetPosition() const;
        glm::vec3 GetForward() const;
        glm::vec3 GetUp() const; // For camera stability
        void Reset(); // Added reset logic

    private:
        gps::Model3D mesh;
        glm::vec3 position;
        
        // Crash Logic
        bool isCrashed = false;
        float verticalVelocity = 0.0f; // For falling
        
        float yaw;
        float pitch;
        float roll;
        float visualTilt; // For VTOL animation

        // Tuning parameters
        float speed;
        float rotSpeed;
        float liftSpeed;
        float tiltSpeed;
        float rollLerpSpeed;
        float turnFactor;

        // Internal rendering helpers
        GLint modelLoc;
        GLint normalMatrixLoc;
    };

}

#endif /* Drone_hpp */
