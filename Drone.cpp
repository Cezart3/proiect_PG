#include "Drone.hpp"
#include "World.hpp" 
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
namespace gps {
    Drone::Drone() {
        position = glm::vec3(0.0f, 2.0f, 0.0f); 
        yaw = 0.0f;
        pitch = 0.0f;
        roll = 0.0f;
        roll = 0.0f;
        visualTilt = 0.0f;
        isBoosting = false;
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
        if (isCrashed) {
            verticalVelocity -= 20.0f * delta; 
            position.y += verticalVelocity * delta;
            pitch += 100.0f * delta;
            roll += 200.0f * delta;
            if (position.y < 2.0f) {
                position.y = 2.0f;
                isCrashed = false; 
                verticalVelocity = 0.0f;
                pitch = 0.0f;
                roll = 0.0f;
                visualTilt = 0.0f;
            }
            return; 
        }
        isBoosting = false;
        if (pressedKeys[GLFW_KEY_F]) {
             isBoosting = true;
        }
        float speedMultiplier = isBoosting ? 2.0f : 1.0f;
        float moveS = speed * speedMultiplier * delta;
        float liftS = liftSpeed * speedMultiplier * delta;
        float rotS = rotSpeed * delta;
        float rollS = 100.0f * delta; 
        float targetRoll = 0.0f;
        float targetVisualTilt = 0.0f; 
        if (pressedKeys[GLFW_KEY_A]) {
            targetRoll = -50.0f; 
        }
        if (pressedKeys[GLFW_KEY_D]) {
            targetRoll = 50.0f;  
        }
        if (pressedKeys[GLFW_KEY_LEFT]) {
            yaw += 40.0f * delta; 
        }
        if (pressedKeys[GLFW_KEY_RIGHT]) {
            yaw -= 40.0f * delta; 
        }
        if (pressedKeys[GLFW_KEY_UP]) {
            pitch -= 50.0f * delta; 
        }
        if (pressedKeys[GLFW_KEY_DOWN]) {
            pitch += 50.0f * delta;
        }
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        if (pressedKeys[GLFW_KEY_SPACE] || pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
            targetRoll *= 0.2f; 
        }
        roll += (targetRoll - roll) * rollLerpSpeed * delta;
        yaw -= (roll / 50.0f) * turnFactor * delta; 
        glm::vec3 proposedMove = glm::vec3(0.0f);
        if (pressedKeys[GLFW_KEY_SPACE]) {
            proposedMove.y += liftS;
            targetVisualTilt = -45.0f; 
        }
        if (pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
            proposedMove.y -= liftS;
            targetVisualTilt = 45.0f; 
        }
        glm::vec3 fwd = GetForward();
        if (pressedKeys[GLFW_KEY_W]) {
            proposedMove += fwd * moveS;
        }
        if (pressedKeys[GLFW_KEY_S]) {
            proposedMove -= fwd * moveS;
        }
        glm::vec3 nextPos = position + proposedMove;
        bool collision = world.CheckCollision(nextPos, 0.5f);
        if (!collision) {
             collision = world.CheckCollision(position, 0.5f);
        }
        if (!collision) {
             position = nextPos;
        } else {
             isCrashed = true;
             verticalVelocity = 0.0f; 
             position -= proposedMove * 2.0f; 
        }
        visualTilt += (targetVisualTilt - visualTilt) * tiltSpeed * delta;
        if(position.y < 2.0f) position.y = 2.0f;
    }
    void Drone::Draw(gps::Shader& shader, glm::mat4 viewMatrix) {
        shader.useShaderProgram();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(yaw), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(pitch), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(roll), glm::vec3(0, 0, 1));
        model = glm::rotate(model, glm::radians(visualTilt), glm::vec3(1, 0, 0));
        model = glm::scale(model, glm::vec3(0.005f)); 
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        GLint modelLoc = glGetUniformLocation(shader.shaderProgram, "model");
        GLint normalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glm::mat3 normMat = glm::mat3(glm::inverseTranspose(viewMatrix * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normMat));
        for(size_t i=0; i<mesh.meshes.size(); ++i) {
            glm::vec3 originalKd = mesh.meshes[i].Kd;
            float brightness = (originalKd.r + originalKd.g + originalKd.b) / 3.0f;
            if (brightness > 0.8f) mesh.meshes[i].Kd = glm::vec3(1.0f, 0.4f, 0.0f); 
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
    glm::vec3 Drone::GetUp() const {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }
}