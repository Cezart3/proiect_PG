#include "Drone.hpp"
#include "World.hpp" // Needed for collision check
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace gps {

    Drone::Drone() {
        position = glm::vec3(0.0f, 2.0f, 0.0f); // Default start
        yaw = 0.0f;
        pitch = 0.0f;
        roll = 0.0f;
        visualTilt = 0.0f;

        // Tuning
        speed = 60.0f;
        rotSpeed = 40.0f;
        liftSpeed = 30.0f;
        tiltSpeed = 5.0f;
        rollLerpSpeed = 4.0f;
        turnFactor = 60.0f;
    }

    void Drone::Load(std::string modelPath) {
        mesh.LoadModel(modelPath);
    }

    void Drone::Update(float delta, GLboolean pressedKeys[], World& world) {
        
        // --- CRASH STATE ---
        if (isCrashed) {
            // physics: Fall
            verticalVelocity -= 20.0f * delta; // Gravity
            position.y += verticalVelocity * delta;
            
            // Visual Tumble
            pitch += 100.0f * delta;
            roll += 200.0f * delta;
            
            // Ground Collision (Recovery)
            if (position.y < 2.0f) {
                position.y = 2.0f;
                isCrashed = false; // Recover
                verticalVelocity = 0.0f;
                
                // Reset orientation to safe levels
                pitch = 0.0f;
                roll = 0.0f;
                visualTilt = 0.0f;
            }
            return; // Skip controls
        }
        
        // --- NORMAL FLIGHT ---

        float moveS = speed * delta;
        float liftS = liftSpeed * delta;
        float rotS = rotSpeed * delta;
        float rollS = 100.0f * delta; 
        
        float targetRoll = 0.0f;
        float targetVisualTilt = 0.0f; 

        // A/D: BANK & TURN
        if (pressedKeys[GLFW_KEY_A]) {
            targetRoll = -50.0f; // Bank Left
        }
        if (pressedKeys[GLFW_KEY_D]) {
            targetRoll = 50.0f;  // Bank Right
        }

        // RUDDER
        if (pressedKeys[GLFW_KEY_LEFT]) {
            yaw += 40.0f * delta; 
        }
        if (pressedKeys[GLFW_KEY_RIGHT]) {
            yaw -= 40.0f * delta; 
        }

        // PITCH
        if (pressedKeys[GLFW_KEY_UP]) {
            pitch -= 50.0f * delta; 
        }
        if (pressedKeys[GLFW_KEY_DOWN]) {
            pitch += 50.0f * delta;
        }

        // CLAMP PITCH to prevent Gimbal Lock and Camera Crashes
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        // VTOL LOGIC (Dominant)
        // If applying vertical thrust, stabilize the drone (limit banking)
        if (pressedKeys[GLFW_KEY_SPACE] || pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
            targetRoll *= 0.2f; // Clamp banking to 20% (approx 10 degrees)
        }

        // AUTO-LEVELING
        roll += (targetRoll - roll) * rollLerpSpeed * delta;

        // AERODYNAMIC TURN
        // Yaw depends on Roll. Small Roll -> Small Turn.
        yaw -= (roll / 50.0f) * turnFactor * delta; 
        
        // Calculate Proposed Movement Vector
        glm::vec3 proposedMove = glm::vec3(0.0f);

        // Vertical
        if (pressedKeys[GLFW_KEY_SPACE]) {
            proposedMove.y += liftS;
            targetVisualTilt = -45.0f; 
        }
        if (pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
            proposedMove.y -= liftS;
            targetVisualTilt = 45.0f; 
        }
        
        // Horizontal
        glm::vec3 fwd = GetForward();
        if (pressedKeys[GLFW_KEY_W]) {
            proposedMove += fwd * moveS;
        }
        if (pressedKeys[GLFW_KEY_S]) {
            proposedMove -= fwd * moveS;
        }
        
        // Apply Movement
        glm::vec3 nextPos = position + proposedMove;
        
        // 1. ACTIVE Collision Check (Did I fly into something?)
        bool collision = world.CheckCollision(nextPos, 3.0f);
        
        // 2. PASSIVE Collision Check (Did something hit me while I was sitting still?)
        // If no active collision, check if the NEW world state (asteroids moved) conflicts with CURRENT position
        if (!collision) {
             collision = world.CheckCollision(position, 3.0f);
        }

        if (!collision) {
             position = nextPos;
        } else {
             // CRASH TRIGGER
             isCrashed = true;
             verticalVelocity = 0.0f; // Reset momentum, start fresh fall
             
             // Bounce back slightly?
             position -= proposedMove * 2.0f; 
        }
        
        // VISUAL TILT SMOOTHING
        visualTilt += (targetVisualTilt - visualTilt) * tiltSpeed * delta;

        // COLLISION with GROUND (Safety)
        if(position.y < 2.0f) position.y = 2.0f;
    }

    void Drone::Draw(gps::Shader& shader, glm::mat4 viewMatrix) {
        shader.useShaderProgram();

        // Calculate Model Matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(yaw), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(pitch), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(roll), glm::vec3(0, 0, 1));
        
        // Visual Tilt
        model = glm::rotate(model, glm::radians(visualTilt), glm::vec3(1, 0, 0));

        // Uniforms
        GLint modelLoc = glGetUniformLocation(shader.shaderProgram, "model");
        GLint normalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glm::mat3 normMat = glm::mat3(glm::inverseTranspose(viewMatrix * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normMat));

        // Render Colored Model
        for(size_t i=0; i<mesh.meshes.size(); ++i) {
            glm::vec3 originalKd = mesh.meshes[i].Kd;
            float brightness = (originalKd.r + originalKd.g + originalKd.b) / 3.0f;
            if (brightness > 0.8f) mesh.meshes[i].Kd = glm::vec3(1.0f, 0.4f, 0.0f); // Orange Player
            
            mesh.meshes[i].Draw(shader);
            
            mesh.meshes[i].Kd = originalKd; 
        }
    }

    glm::vec3 Drone::GetPosition() const {
        return position;
    }

    glm::vec3 Drone::GetForward() const {
        glm::mat4 rotMat = glm::mat4(1.0f);
        rotMat = glm::rotate(rotMat, glm::radians(yaw), glm::vec3(0, 1, 0));
        rotMat = glm::rotate(rotMat, glm::radians(pitch), glm::vec3(1, 0, 0));
        rotMat = glm::rotate(rotMat, glm::radians(roll), glm::vec3(0, 0, 1));
        glm::vec4 fwd4 = rotMat * glm::vec4(0, 0, 1, 0); 
        return glm::normalize(glm::vec3(fwd4));
    }
    
    // Helper helper for camera: Forward independent of Roll
    glm::vec3 Drone::GetUp() const {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

}
