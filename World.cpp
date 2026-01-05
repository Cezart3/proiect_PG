#include "World.hpp"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp> // VITAL: For normal matrix calculation
#include <iostream>

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
        
        // Define Obstacles (matching RenderMesh calls)
        // Pos: (100, 0, 100), Scale: 50 -> Radius approx 20
        obstacles.push_back({glm::vec3(100.0f, 0.0f, 100.0f), 20.0f * 1.5f}); // A bit larger for safety
        
        // Pos: (200, 0, -150), Scale: 80
        obstacles.push_back({glm::vec3(200.0f, 0.0f, -150.0f), 30.0f * 1.5f});
        
        // Pos: (-150, 0, 120), Scale: 60
        obstacles.push_back({glm::vec3(-150.0f, 0.0f, 120.0f), 25.0f * 1.5f});
    }

    bool World::CheckCollision(glm::vec3 position, float radius) {
        for (const auto& obs : obstacles) {
            float dist = glm::distance(position, obs.position);
            // If distance < sum of radii, collision!
            if (dist < (obs.radius + radius)) {
                return true;
            }
        }
        return false;
    }



    void World::Draw(gps::Shader& basicShader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
        
        basicShader.useShaderProgram();

        // 0. SEND LIGHTS (Advanced Lighting)
        // Transform lights to Eye Space!
        // For Directional Light (passed from Main currently), main does the transform?
        // Let's assume Main handles DirLight for now.
        
        // Point Lights (Crystals)
        // Let's fake one crystal in a crater for now
        glm::vec3 crystalPos = glm::vec3(-100.0f, 5.0f, -100.0f); // Inside Crater 1
        glm::vec3 crystalPosEye = glm::vec3(viewMatrix * glm::vec4(crystalPos, 1.0f));
        
        glUniform3fv(glGetUniformLocation(basicShader.shaderProgram, "pointLights[0].position"), 1, glm::value_ptr(crystalPosEye));
        glUniform3fv(glGetUniformLocation(basicShader.shaderProgram, "pointLights[0].color"), 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f))); // Green
        glUniform1f(glGetUniformLocation(basicShader.shaderProgram, "pointLights[0].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(basicShader.shaderProgram, "pointLights[0].linear"), 0.045f);
        glUniform1f(glGetUniformLocation(basicShader.shaderProgram, "pointLights[0].quadratic"), 0.0075f);
        
        glUniform1i(glGetUniformLocation(basicShader.shaderProgram, "nrPointLights"), 1); // 1 Active
        

        // SPOT LIGHT (Drone Headlight) is handled in Main (dynamic pos) or we pass it here?
        // Main calls World::Draw, then Drone::Draw.
        // It's cleaner to set global lighting uniforms ONCE per frame in Main.
        // But World handles Environment lights...
        // Let's leave SpotLight for Main to set, as it tracks the Drone.
        
        // ... (RenderMesh calls) ...
        
        // ROCKS
        RenderMesh(rock, basicShader, viewMatrix, projectionMatrix, glm::vec3(100.0f, 0.0f, 100.0f), 0.0f, 50.0f, glm::vec3(0.6f));
        RenderMesh(rock, basicShader, viewMatrix, projectionMatrix, glm::vec3(200.0f, 0.0f, -150.0f), 90.0f, 80.0f, glm::vec3(0.5f));
        RenderMesh(rock, basicShader, viewMatrix, projectionMatrix, glm::vec3(-150.0f, 0.0f, 120.0f), 180.0f, 60.0f, glm::vec3(0.7f));
        
        // CRATERS
        RenderMesh(crater, basicShader, viewMatrix, projectionMatrix, glm::vec3(-100.0f, -5.0f, -100.0f), 0.0f, 30.0f, glm::vec3(1.0f));
        RenderMesh(crater, basicShader, viewMatrix, projectionMatrix, glm::vec3(250.0f, -5.0f, 250.0f), 45.0f, 40.0f, glm::vec3(1.0f));

        // 2. Draw Ground
        ground.Draw(basicShader, viewMatrix); // Ground usually handles its own drawing but might need help if it doesn't set uniforms?
        // Note: Ground.hpp usually just binds texture and draws plane.
        
        // 3. Draw Skybox
        skyBox.Draw(skyboxShader, viewMatrix, projectionMatrix);
    }

    void World::RenderMesh(gps::Model3D &mesh, gps::Shader& shader, glm::mat4 view, glm::mat4 projection, 
                          glm::vec3 position, float rotationAngle, float scale, glm::vec3 colorOverride) {
        
        shader.useShaderProgram();
        
        // Uniform Locations (Assume standard names)
        GLint modelLoc = glGetUniformLocation(shader.shaderProgram, "model");
        GLint normalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
        // View/Proj should be set by Caller (RenderScene) globally optimization, 
        // but here we can't assume they are up to date if we switched shaders?
        // BasicShader is passed in. We assume View/Proj are valid or we explicitly set them?
        // In main.cpp we usually set View/Proj once per frame. 
        // But let's assume standard behavior. normalMatrix depends on View * Model.
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(scale));
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glm::mat3 normMat = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normMat));

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
