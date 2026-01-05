#include "World.hpp"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp> // VITAL: For normal matrix calculation
#include <iostream>
#include <GLFW/glfw3.h>
#include <cstdlib> // rand

namespace gps {

    World::World() {
    }

    void World::Init() {
        // Load Assets
        ground.Load("textures/ground.png");
        
        rock.LoadModel("models/kenney_space-kit/Models/OBJ format/rock_largeA.obj");
        crater.LoadModel("models/kenney_space-kit/Models/OBJ format/craterLarge.obj");
        
        // Initialize Skybox Shader & Texture
        skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
        
        std::vector<std::string> faces;
        faces.push_back("textures/skybox/right.png");
        faces.push_back("textures/skybox/left.png");
        faces.push_back("textures/skybox/top.png");
        faces.push_back("textures/skybox/bottom.png");
        faces.push_back("textures/skybox/back.png");
        faces.push_back("textures/skybox/front.png");
        skyBox.Load(faces);
        
        // Define Fixed Obstacles
        obstacles.push_back({glm::vec3(100.0f, 0.0f, 100.0f), 30.0f});
        obstacles.push_back({glm::vec3(200.0f, 0.0f, -150.0f), 45.0f});
        obstacles.push_back({glm::vec3(-150.0f, 0.0f, 120.0f), 35.0f});
        
        // --- CONTENT GENERATION ---
        
        // 1. ASTEROIDS (Populated in Update)
        // Just reserve if needed, or leave empty until first Update call
        
        // 2. SPIRES (Tall thin structures)
        // Random placement
        srand(42); // Seed for consistency
        for(int i=0; i<20; ++i) {
            float x = (rand() % 800) - 400.0f;
            float z = (rand() % 800) - 400.0f;
            spirePositions.push_back(glm::vec3(x, 0.0f, z));
            
            // Removed from 'obstacles' to avoid double checking as Sphere
            // Handled as Cylinder in CheckCollision directly via spirePositions
        }
    }

    void World::Update(float delta) {
        // Update Asteroid Positions & Collision
        asteroidPositions.clear();
        dynamicObstacles.clear();
        
        float time = (float)glfwGetTime();
        
        for(int i=0; i<15; ++i) {
            float angleBase = (float)i * (360.0f / 15.0f);
            float orbitSpeed = 0.1f + (i * 0.01f);
            float angle = glm::radians(angleBase) + (time * orbitSpeed); // Continuous orbit
            
            float radius = 250.0f;
            float x = radius * cos(angle); // Fix logic: x = r*cos, z = r*sin for circle
            float z = radius * sin(angle);
            float y = 150.0f + ((i % 2 == 0) ? 30.0f : -30.0f); 
            
            glm::vec3 pos = glm::vec3(x, y, z);
            asteroidPositions.push_back(pos);
            
            // Add to Dynamic Obstacles for collision
            dynamicObstacles.push_back({pos, 25.0f}); // Radius matched visually
        }
    }

    bool World::CheckCollision(glm::vec3 position, float radius) {
        // 1. Static Obstacles (Spheres)
        for (const auto& obs : obstacles) {
            float dist = glm::distance(position, obs.position);
            if (dist < (radius + obs.radius)) {
                return true;
            }
        }
        
        // 2. Dynamic Obstacles (Asteroids - Spheres)
        for (const auto& obs : dynamicObstacles) {
            float dist = glm::distance(position, obs.position);
            if (dist < (radius + obs.radius)) {
                return true;
            }
        }
        
        // 3. Spires (Vertical Cylinders)
        // Height check + XZ Distance check
        float spireHeight = 160.0f; // Approx visual height (Scale Y=80 * 2 for model size?)
        float spireRadius = 15.0f;  // Scale X/Z=15
        
        if (position.y < spireHeight) {
            for (const auto& spirePos : spirePositions) {
                // XZ Distance
                float dx = position.x - spirePos.x;
                float dz = position.z - spirePos.z;
                float distXZ = sqrt(dx*dx + dz*dz);
                
                if (distXZ < (spireRadius + radius)) { // Simple cylinder check
                    return true;
                }
            }
        }
        
        return false;
    }



    void World::Draw(gps::Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, RenderType type) {
        
        shader.useShaderProgram();

        // 0. SEND LIGHTS (Only for Normal Render)
        if (type == RENDER_ALL) {
            // Point Lights (Crystals)
            glm::vec3 crystalPos = glm::vec3(-100.0f, 5.0f, -100.0f); // Inside Crater 1
            glm::vec3 crystalPosEye = glm::vec3(viewMatrix * glm::vec4(crystalPos, 1.0f));
            
            // Check if uniform exists before setting (Optimization/Safety)
            GLint loc = glGetUniformLocation(shader.shaderProgram, "pointLights[0].position");
            if (loc >= 0) {
                glUniform3fv(loc, 1, glm::value_ptr(crystalPosEye));
                glUniform3fv(glGetUniformLocation(shader.shaderProgram, "pointLights[0].color"), 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f))); // Green
                glUniform1f(glGetUniformLocation(shader.shaderProgram, "pointLights[0].constant"), 1.0f);
                glUniform1f(glGetUniformLocation(shader.shaderProgram, "pointLights[0].linear"), 0.045f);
                glUniform1f(glGetUniformLocation(shader.shaderProgram, "pointLights[0].quadratic"), 0.0075f);
                glUniform1i(glGetUniformLocation(shader.shaderProgram, "nrPointLights"), 1);
            }
        }
        
        // 1. ROCKS & CRATERS (Originals)
        RenderMesh(rock, shader, viewMatrix, projectionMatrix, glm::vec3(100.0f, 0.0f, 100.0f), 0.0f, 50.0f, glm::vec3(0.6f));
        RenderMesh(rock, shader, viewMatrix, projectionMatrix, glm::vec3(200.0f, 0.0f, -150.0f), 90.0f, 80.0f, glm::vec3(0.5f));
        RenderMesh(rock, shader, viewMatrix, projectionMatrix, glm::vec3(-150.0f, 0.0f, 120.0f), 180.0f, 60.0f, glm::vec3(0.7f));
        
        RenderMesh(crater, shader, viewMatrix, projectionMatrix, glm::vec3(-100.0f, -5.0f, -100.0f), 0.0f, 30.0f, glm::vec3(1.0f));
        RenderMesh(crater, shader, viewMatrix, projectionMatrix, glm::vec3(250.0f, -5.0f, 250.0f), 45.0f, 40.0f, glm::vec3(1.0f));

        // 2. SPIRES (New Tall Objects)
        for(const auto& pos : spirePositions) {
             // Create a "Tower" shape by scaling Y axis significantly
             // Scale X/Z: 15.0f, Scale Y: 80.0f
             RenderMesh(rock, shader, viewMatrix, projectionMatrix, pos, 0.0f, glm::vec3(15.0f, 80.0f, 15.0f), glm::vec3(0.4f, 0.4f, 0.5f));
        }

        // 3. ASTEROIDS (Floating & Orbiting)
        // Positions are updated in World::Update()
        float time = (float)glfwGetTime();
        for(size_t i=0; i<asteroidPositions.size(); ++i) {
            glm::vec3 pos = asteroidPositions[i]; 
            
            // Rotation/Tumbe only (Position is already done)
            float tumble = time * 20.0f;
            
            RenderMesh(rock, shader, viewMatrix, projectionMatrix, pos, tumble, 20.0f + (i % 10), glm::vec3(0.6f, 0.5f, 0.4f));
        }

        // 4. Draw Ground
        ground.Draw(shader, viewMatrix); 
        
        // 5. Draw Skybox (Only for Normal Render)
        if (type == RENDER_ALL) {
            skyBox.Draw(skyboxShader, viewMatrix, projectionMatrix);
        }
    }

    void World::RenderMesh(gps::Model3D &mesh, gps::Shader& shader, glm::mat4 view, glm::mat4 projection, 
                          glm::vec3 position, float rotationAngle, float scale, glm::vec3 colorOverride) {
       // Call overload
       RenderMesh(mesh, shader, view, projection, position, rotationAngle, glm::vec3(scale), colorOverride);
    }
    
    void World::RenderMesh(gps::Model3D &mesh, gps::Shader& shader, glm::mat4 view, glm::mat4 projection, 
                          glm::vec3 position, float rotationAngle, glm::vec3 scaleVector, glm::vec3 colorOverride) {
        
        shader.useShaderProgram();
        
        // Uniform Locations (Assume standard names)
        GLint modelLoc = glGetUniformLocation(shader.shaderProgram, "model");
        GLint normalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0, 1, 0));
        model = glm::scale(model, scaleVector);
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        
        // Optimize: Only calc normal matrix if loc is valid (not depth shader)
        if(normalMatrixLoc >= 0) {
            glm::mat3 normMat = glm::mat3(glm::inverseTranspose(view * model));
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normMat));
        }

        for(size_t i=0; i<mesh.meshes.size(); ++i) {
            glm::vec3 originalKd = mesh.meshes[i].Kd;
            
            // Tinting logic (copied from main)
            // If colorOverride is not white (1,1,1), apply it loosely
            if (colorOverride != glm::vec3(1.0f)) {
                 mesh.meshes[i].Kd = colorOverride;
            }
            
            mesh.meshes[i].Draw(shader);
            mesh.meshes[i].Kd = originalKd; 
        }
    }

}
