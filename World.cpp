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

        // 3. PROCEDURAL CITY (Realism Phase: Denser & Varied)
        building.LoadModel("models/kenney_space-kit/Models/OBJ format/hangar_largeA.obj");
        alien.LoadModel("models/kenney_space-kit/Models/OBJ format/alien.obj");
        sun.LoadModel("models/kenney_space-kit/Models/OBJ format/rock_largeA.obj"); // Reuse rock as sun base
        
        // New Models
        tower1.LoadModel("models/tower1/base.obj");
        tower2.LoadModel("models/tower2/base.obj");
        newAlien.LoadModel("models/new_alien/base.obj");
        
        int numBuildings = 200; // Increased density from 150
        for(int i=0; i<numBuildings; ++i) {
            // Random Position
            float x = (rand() % 2400) - 1200.0f;
            float z = (rand() % 2400) - 1200.0f;
            
            // Avoid Center (Spawn area)
            if (std::abs(x) < 200.0f && std::abs(z) < 200.0f) continue;
            
            // Avoid Overlap (Simple check)
            bool collision = false;
            for(const auto& b : cityBuildings) {
                if (glm::distance(b.position, glm::vec3(x, 0.1f, z)) < 150.0f) { // Increased spacing for larger buildings
                    collision = true;
                    break;
                }
            }
            if(collision) continue;
            
            BuildingInstance inst;
            inst.position = glm::vec3(x, 0.1f, z);
            inst.rotation = (float)(rand() % 360);
            
            // Random Type Selection
            int rType = rand() % 100;
            if (rType < 40) inst.type = 0; // Hangar (40%)
            else if (rType < 70) inst.type = 1; // Tower1 (30%)
            else inst.type = 2; // Tower2 (30%)

            // Random Color
            int colorType = rand() % 4;
            if (colorType == 0) inst.color = glm::vec3(0.8f, 0.8f, 0.9f); // Std
            else if (colorType == 1) inst.color = glm::vec3(0.5f, 0.6f, 0.8f); // Blueish
            else if (colorType == 2) inst.color = glm::vec3(0.7f, 0.7f, 0.7f); // Darker
            else inst.color = glm::vec3(0.9f, 0.8f, 0.7f); // Sand/Rust
            
            // Apply Properties based on Type
            if (inst.type == 0) {
                 // Hangar
                 inst.scale = glm::vec3(15.0f, 15.0f, 15.0f);
                 
                 // Spawn Aliens! (Formation of 3)
                 glm::mat4 m = glm::mat4(1.0f);
                 m = glm::rotate(m, glm::radians(inst.rotation), glm::vec3(0,1,0));
                 glm::vec3 fwd = glm::vec3(m * glm::vec4(0, 0, 1, 0));
                 
                 int alienType = rand() % 2; // Random alien type for this group
                 
                 for(int a=0; a<3; ++a) {
                     glm::vec3 alienPos = inst.position + (fwd * 80.0f) + (glm::vec3(m * glm::vec4(1,0,0,0)) * (float)(a-1) * 25.0f);
                     alienPos.y = 0.0f;
                     alienInstances.push_back({alienPos, alienType});
                 }

            } else if (inst.type == 1) {
                 // Tower 1 (Medium) -> Larger
                 inst.scale = glm::vec3(20.0f, 20.0f, 20.0f);
            } else {
                 // Tower 2 (Large) -> Massive
                 inst.scale = glm::vec3(40.0f, 40.0f, 40.0f);
            }
            // Removed manual obstacles.push_back checks are handled in CheckCollision custom logic now
            
            cityBuildings.push_back(inst);
        }

        // 4. SCATTERED NEW ALIENS
        // Randomly scatter 50 new aliens globally
        for(int i=0; i<50; ++i) {
             float x = (rand() % 2400) - 1200.0f;
             float z = (rand() % 2400) - 1200.0f;
             
             // Avoid Center
             if (std::abs(x) < 200.0f && std::abs(z) < 200.0f) continue;

             alienInstances.push_back({glm::vec3(x, 10.0f, z), 1}); // Type 1 = New Alien
        }

        // 5. ALIEN CITY (Mini City)
        // Center: -800, -800. Radius: 300
        int alienCityDensity = 80;
        float cityCx = -800.0f;
        float cityCz = -800.0f;
        
        for(int i=0; i<alienCityDensity; ++i) {
             float angle = (float)(rand() % 360);
             float dist = (float)(rand() % 300);
             float x = cityCx + dist * cos(glm::radians(angle));
             float z = cityCz + dist * sin(glm::radians(angle));
             
             BuildingInstance inst;
             inst.position = glm::vec3(x, 0.1f, z);
             inst.rotation = (float)(rand() % 360);
             inst.color = glm::vec3(1.0f); // Default for textures
             
             // Only New Towers
             if (rand() % 2 == 0) {
                 inst.type = 1; // Tower 1
                 inst.scale = glm::vec3(20.0f, 20.0f, 20.0f);
             } else {
                 inst.type = 2; // Tower 2
                 inst.scale = glm::vec3(40.0f, 40.0f, 40.0f);
             }
             
             cityBuildings.push_back(inst);
             
             // Add Defenders (New Aliens)
             if (i % 3 == 0) {
                 alienInstances.push_back({glm::vec3(x, 20.0f, z), 1});
             }
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
            // Raised Height: was 150 +/- 30. Now 300 +/- 50
            float y = 300.0f + ((i % 2 == 0) ? 50.0f : -50.0f); 
            
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
        
        // 2. Asteroids (Sphere)
        float asteroidBaseRadius = 4.0f; // Visual size approx
        for (const auto& asteroid : dynamicObstacles) {
            // Assuming asteroid.radius is the base scale factor for the model
            // The model's actual radius is scaled by this value.
            // The original code used 25.0f as obs.radius, which was the visual radius.
            // To reduce it, we can use a smaller multiplier or a fixed smaller value.
            // The instruction implies a calculation based on a scale factor, but dynamicObstacles only stores position and a single radius.
            // Let's interpret "asteroid.scale.x * asteroidBaseRadius * 0.8f" as a new, reduced effective radius for collision.
            // Since dynamicObstacles stores {pos, 25.0f}, we'll use the 25.0f as a base scale.
            float combinedRadius = asteroid.radius * 0.8f + radius; // Reduced multiplier from 1.0 to 0.8
            float dist = glm::length(position - asteroid.position);
            if (dist < combinedRadius) {
                return true;
            }
        }

        // 3. Spires (Vertical Cylinders)
        float spireHeight = 160.0f;
        float spireRadius = 6.0f; // Visual width is thinner than base
        
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
        
        // 4. City Buildings (Cylinders/Boxes)
        float playerHeight = 2.0f; // Approx height of camera/player
        for (const auto& building : cityBuildings) {
            // Calculate effective bounds
            float buildingRadius = building.scale.x * 2.5f; // Matches visual approximate width
            float buildingHeight = building.scale.y * 2.0f; // Matches visual approximate height
            
            // Height Check: If we are ABOVE the building, no collision
            // building.position.y is 0.1f. 
            if (position.y > (building.position.y + buildingHeight + 1.0f)) {
                 continue;
            }
            
            // Radius Check (XZ Plane)
            float dx = position.x - building.position.x;
            float dz = position.z - building.position.z;
            float distXZ = sqrt(dx*dx + dz*dz);
            
            if (distXZ < (buildingRadius + radius)) {
                return true;
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

        // 3. CITY (Unified)
        for(const auto& inst : cityBuildings) {
            if (inst.type == 0) {
                RenderMesh(building, shader, viewMatrix, projectionMatrix, inst.position, inst.rotation, inst.scale, inst.color);
            } else if (inst.type == 1) {
                RenderMesh(tower1, shader, viewMatrix, projectionMatrix, inst.position, inst.rotation, inst.scale, inst.color);
            } else if (inst.type == 2) {
                RenderMesh(tower2, shader, viewMatrix, projectionMatrix, inst.position, inst.rotation, inst.scale, inst.color);
            }
        }
        
        // 4. ALIENS
        for(const auto& alienInst : alienInstances) {
             if (alienInst.type == 0) {
                 RenderMesh(alien, shader, viewMatrix, projectionMatrix, alienInst.position, 0.0f, 8.0f, glm::vec3(0.2f, 0.8f, 0.2f));
             } else {
                 // New Alien (Larger)
                 RenderMesh(newAlien, shader, viewMatrix, projectionMatrix, alienInst.position, 0.0f, 12.0f, glm::vec3(1.0f)); 
             }
        }
        
        // 5. SUN (Visible Source)
        // Position matches LightDir loc or fixed sun position
        // Only render in normal pass to avoid shadow weirdness (or keep it, it blocks light?)
        if (type == RENDER_ALL) {
             // Sun Position: Matches main.cpp lightDir (0, 10, 10) -> Far away direction
             // Let's put a physical object there to look at.
             // We need it to be "Infinite" but for visual we place it far.
             // Direction is approx (0, 1, 1).
             glm::vec3 sunPos = glm::vec3(0.0f, 500.0f, 500.0f); 
             RenderMesh(sun, shader, viewMatrix, projectionMatrix, sunPos, 0.0f, 30.0f, glm::vec3(1.0f, 1.0f, 0.5f)); // Yellow-ish
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
