#include "World.hpp"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp> 
#include <iostream>
#include <GLFW/glfw3.h>
#include <cstdlib> 
namespace gps {
    World::World() {
    }
    void World::Init() {
        ground.Load("textures/ground.png");
        rock.LoadModel("models/kenney_space-kit/Models/OBJ format/rock_largeA.obj");
        crater.LoadModel("models/kenney_space-kit/Models/OBJ format/craterLarge.obj");
        skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
        std::vector<std::string> faces;
        faces.push_back("textures/skybox/right.png");
        faces.push_back("textures/skybox/left.png");
        faces.push_back("textures/skybox/top.png");
        faces.push_back("textures/skybox/bottom.png");
        faces.push_back("textures/skybox/back.png");
        faces.push_back("textures/skybox/front.png");
        skyBox.Load(faces);
        obstacles.push_back({glm::vec3(100.0f, 0.0f, 100.0f), 30.0f});
        obstacles.push_back({glm::vec3(200.0f, 0.0f, -150.0f), 45.0f});
        obstacles.push_back({glm::vec3(-150.0f, 0.0f, 120.0f), 35.0f});
        srand(42); 
        for(int i=0; i<20; ++i) {
            float x = (rand() % 800) - 400.0f;
            float z = (rand() % 800) - 400.0f;
            spirePositions.push_back(glm::vec3(x, 0.0f, z));
        }
        building.LoadModel("models/kenney_space-kit/Models/OBJ format/hangar_largeA.obj");
        alien.LoadModel("models/kenney_space-kit/Models/OBJ format/alien.obj");
        sun.LoadModel("models/kenney_space-kit/Models/OBJ format/rock_largeA.obj"); 
        nitroModel.LoadModel("models/kenney_space-kit/Models/OBJ format/rocket_fuelA.obj"); 
        tower1.LoadModel("models/tower1/base.obj");
        tower2.LoadModel("models/tower2/base.obj");
        newAlien.LoadModel("models/new_alien/base.obj");
        int numBuildings = 200; 
        for(int i=0; i<numBuildings; ++i) {
            float x = (rand() % 2400) - 1200.0f;
            float z = (rand() % 2400) - 1200.0f;
            if (std::abs(x) < 200.0f && std::abs(z) < 200.0f) continue;
            bool collision = false;
            for(const auto& b : cityBuildings) {
                if (glm::distance(b.position, glm::vec3(x, 0.1f, z)) < 150.0f) { 
                    collision = true;
                    break;
                }
            }
            if(collision) continue;
            BuildingInstance inst;
            inst.position = glm::vec3(x, 0.1f, z);
            inst.rotation = (float)(rand() % 360);
            int rType = rand() % 100;
            if (rType < 40) inst.type = 0; 
            else if (rType < 70) inst.type = 1; 
            else inst.type = 2; 
            int colorType = rand() % 4;
            if (colorType == 0) inst.color = glm::vec3(0.8f, 0.8f, 0.9f); 
            else if (colorType == 1) inst.color = glm::vec3(0.5f, 0.6f, 0.8f); 
            else if (colorType == 2) inst.color = glm::vec3(0.7f, 0.7f, 0.7f); 
            else inst.color = glm::vec3(0.9f, 0.8f, 0.7f); 
            if (inst.type == 0) {
                 inst.scale = glm::vec3(15.0f, 15.0f, 15.0f);
                 inst.health = 20;
                 glm::mat4 m = glm::mat4(1.0f);
                 m = glm::rotate(m, glm::radians(inst.rotation), glm::vec3(0,1,0));
                 glm::vec3 fwd = glm::vec3(m * glm::vec4(0, 0, 1, 0));
                 int alienType = rand() % 2; 
                 for(int a=0; a<3; ++a) {
                     glm::vec3 alienPos = inst.position + (fwd * 80.0f) + (glm::vec3(m * glm::vec4(1,0,0,0)) * (float)(a-1) * 25.0f);
                     alienPos.y = 0.0f;
                     alienInstances.push_back({alienPos, alienType, 4}); 
                 }
            } else if (inst.type == 1) {
                 inst.scale = glm::vec3(20.0f, 20.0f, 20.0f);
                 inst.health = 20;
            } else {
                 inst.scale = glm::vec3(40.0f, 40.0f, 40.0f);
                 inst.health = 50;
            }
            cityBuildings.push_back(inst);
        }
        for(int i=0; i<50; ++i) {
             float x = (rand() % 2400) - 1200.0f;
             float z = (rand() % 2400) - 1200.0f;
             if (std::abs(x) < 200.0f && std::abs(z) < 200.0f) continue;
             alienInstances.push_back({glm::vec3(x, 10.0f, z), 1, 4}); 
        }
        int numBuildingsRing = 800; 
        float ringRadius = 1800.0f; 
        float ringWidth = 500.0f; 
        for (int i = 0; i < numBuildingsRing; ++i) {
            float angle = (float)i * (360.0f / numBuildingsRing);
            angle += ((rand() % 100) / 100.0f * 5.0f); 
            float distOffset = ((rand() % 100) / 100.0f * ringWidth) - (ringWidth * 0.5f);
            float r = ringRadius + distOffset;
            float x = r * cos(glm::radians(angle));
            float z = r * sin(glm::radians(angle));
            BuildingInstance inst;
            inst.position = glm::vec3(x, 0.1f, z);
            inst.rotation = (float)(rand() % 360);
            inst.color = glm::vec3(1.0f); 
            if (rand() % 10 < 3) { 
                 inst.type = 0; 
                 inst.scale = glm::vec3(15.0f, 15.0f, 15.0f);
                 inst.health = 20;
            } else if (rand() % 2 == 0) { 
                 inst.type = 1;
                 inst.scale = glm::vec3(20.0f, 20.0f, 20.0f);
                 inst.health = 20;
            } else { 
                 inst.type = 2;
                 inst.scale = glm::vec3(40.0f, 40.0f, 40.0f);
                 inst.health = 50; 
            }
            cityBuildings.push_back(inst);
            if (i % 5 == 0) {
                 alienInstances.push_back({glm::vec3(x, 20.0f, z), 1, 4}); 
            }
        }
    }
    void World::Update(float delta) {
        asteroidPositions.clear();
        dynamicObstacles.clear();
        float time = (float)glfwGetTime();
        for(int i=0; i<15; ++i) {
            float angleBase = (float)i * (360.0f / 15.0f);
            float orbitSpeed = 0.1f + (i * 0.01f);
            float angle = glm::radians(angleBase) + (time * orbitSpeed); 
            float radius = 250.0f;
            float x = radius * cos(angle);
            float z = radius * sin(angle);
            float y = 300.0f + ((i % 2 == 0) ? 50.0f : -50.0f); 
            glm::vec3 pos = glm::vec3(x, y, z);
            asteroidPositions.push_back(pos);
            dynamicObstacles.push_back({pos, 25.0f});
        }
        for (int i = 0; i < bullets.size(); ) {
            bullets[i].position += bullets[i].velocity * delta;
            bullets[i].life -= delta;
            if (bullets[i].life < 0) {
                bullets.erase(bullets.begin() + i);
                continue;
            }
            bool hit = false;
            for (int b = 0; b < cityBuildings.size(); ++b) {
                float buildingRadius = cityBuildings[b].scale.x * 2.5f; 
                float buildingHeight = cityBuildings[b].scale.y * 2.0f;
                if (bullets[i].position.y < 0.0f || bullets[i].position.y > (cityBuildings[b].position.y + buildingHeight)) {
                     continue; 
                }
                float distXZ = glm::distance(glm::vec2(bullets[i].position.x, bullets[i].position.z), 
                                             glm::vec2(cityBuildings[b].position.x, cityBuildings[b].position.z));
                if (distXZ < buildingRadius) {
                    cityBuildings[b].health--;
                    hit = true;
                    if (cityBuildings[b].health <= 0) {
                        cityBuildings.erase(cityBuildings.begin() + b);
                    }
                    break;
                }
            }
            if (!hit) {
                for (int a = 0; a < alienInstances.size(); ++a) {
                    float alienRadius = 8.0f;
                    if (alienInstances[a].type == 1) alienRadius = 15.0f; 
                    float dist = glm::distance(bullets[i].position, alienInstances[a].position);
                    if (dist < alienRadius) {
                        alienInstances[a].health--;
                        hit = true;
                        if (alienInstances[a].health <= 0) {
                            alienInstances.erase(alienInstances.begin() + a);
                        }
                        break;
                    }
                }
            }
            if (hit) {
                bullets.erase(bullets.begin() + i);
            } else {
                i++;
            }
        }
    }
    void World::FireBullet(glm::vec3 position, glm::vec3 direction) {
        Bullet b;
        b.position = position;
        b.velocity = glm::normalize(direction) * 400.0f; 
        b.life = 3.0f; 
        bullets.push_back(b);
    }
    bool World::CheckCollision(glm::vec3 position, float radius) {
        for (const auto& obs : obstacles) {
            float dist = glm::distance(position, obs.position);
            if (dist < (radius + obs.radius)) {
                return true;
            }
        }
        float asteroidBaseRadius = 4.0f; 
        for (const auto& asteroid : dynamicObstacles) {
            float combinedRadius = asteroid.radius * 0.8f + radius; 
            float dist = glm::length(position - asteroid.position);
            if (dist < combinedRadius) {
                return true;
            }
        }
        float spireHeight = 160.0f;
        float spireRadius = 6.0f; 
        if (position.y < spireHeight) {
            for (const auto& spirePos : spirePositions) {
                float dx = position.x - spirePos.x;
                float dz = position.z - spirePos.z;
                float distXZ = sqrt(dx*dx + dz*dz);
                if (distXZ < (spireRadius + radius)) { 
                    return true;
                }
            }
        }
        float playerHeight = 2.0f; 
        for (const auto& building : cityBuildings) {
            float buildingRadius = building.scale.x * 0.8f; 
            float buildingHeight = building.scale.y * 1.5f; 
            if (position.y > (building.position.y + buildingHeight + 1.0f)) {
                 continue;
            }
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
        if (type == RENDER_ALL) {
            glm::vec3 crystalPos = glm::vec3(-100.0f, 5.0f, -100.0f); 
            glm::vec3 crystalPosEye = glm::vec3(viewMatrix * glm::vec4(crystalPos, 1.0f));
            GLint loc = glGetUniformLocation(shader.shaderProgram, "pointLights[0].position");
            if (loc >= 0) {
                glUniform3fv(loc, 1, glm::value_ptr(crystalPosEye));
                glUniform3fv(glGetUniformLocation(shader.shaderProgram, "pointLights[0].color"), 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f))); 
                glUniform1f(glGetUniformLocation(shader.shaderProgram, "pointLights[0].constant"), 1.0f);
                glUniform1f(glGetUniformLocation(shader.shaderProgram, "pointLights[0].linear"), 0.045f);
                glUniform1f(glGetUniformLocation(shader.shaderProgram, "pointLights[0].quadratic"), 0.0075f);
                glUniform1i(glGetUniformLocation(shader.shaderProgram, "nrPointLights"), 1);
            }
        }
        RenderMesh(rock, shader, viewMatrix, projectionMatrix, glm::vec3(100.0f, 0.0f, 100.0f), 0.0f, 50.0f, glm::vec3(0.6f));
        RenderMesh(rock, shader, viewMatrix, projectionMatrix, glm::vec3(200.0f, 0.0f, -150.0f), 90.0f, 80.0f, glm::vec3(0.5f));
        RenderMesh(rock, shader, viewMatrix, projectionMatrix, glm::vec3(-150.0f, 0.0f, 120.0f), 180.0f, 60.0f, glm::vec3(0.7f));
        RenderMesh(crater, shader, viewMatrix, projectionMatrix, glm::vec3(-100.0f, -5.0f, -100.0f), 0.0f, 30.0f, glm::vec3(1.0f));
        RenderMesh(crater, shader, viewMatrix, projectionMatrix, glm::vec3(250.0f, -5.0f, 250.0f), 45.0f, 40.0f, glm::vec3(1.0f));
        for(const auto& pos : spirePositions) {
             RenderMesh(rock, shader, viewMatrix, projectionMatrix, pos, 0.0f, glm::vec3(15.0f, 80.0f, 15.0f), glm::vec3(0.4f, 0.4f, 0.5f));
        }
        for(const auto& inst : cityBuildings) {
            if (inst.type == 0) {
                RenderMesh(building, shader, viewMatrix, projectionMatrix, inst.position, inst.rotation, inst.scale, inst.color);
            } else if (inst.type == 1) {
                RenderMesh(tower1, shader, viewMatrix, projectionMatrix, inst.position, inst.rotation, inst.scale, inst.color);
            } else if (inst.type == 2) {
                RenderMesh(tower2, shader, viewMatrix, projectionMatrix, inst.position, inst.rotation, inst.scale, inst.color);
            }
        }
        for(const auto& alienInst : alienInstances) {
             if (alienInst.type == 0) {
                 RenderMesh(alien, shader, viewMatrix, projectionMatrix, alienInst.position, 0.0f, 8.0f, glm::vec3(0.2f, 0.8f, 0.2f));
             } else {
                 RenderMesh(newAlien, shader, viewMatrix, projectionMatrix, alienInst.position, 0.0f, 12.0f, glm::vec3(1.0f)); 
             }
        }
        for(const auto& b : bullets) {
             RenderMesh(sun, shader, viewMatrix, projectionMatrix, b.position, b.velocity, glm::vec3(0.5f, 0.5f, 6.0f), glm::vec3(0.0f, 1.0f, 1.0f)); 
        }
        if (type == RENDER_ALL) {
             glm::vec3 sunPos = glm::vec3(0.0f, 500.0f, 500.0f); 
             RenderMesh(sun, shader, viewMatrix, projectionMatrix, sunPos, 0.0f, 30.0f, glm::vec3(1.0f, 1.0f, 0.5f)); 
        }
        float time = (float)glfwGetTime();
        for(size_t i=0; i<asteroidPositions.size(); ++i) {
            glm::vec3 pos = asteroidPositions[i]; 
            float tumble = time * 20.0f;
            RenderMesh(rock, shader, viewMatrix, projectionMatrix, pos, tumble, 20.0f + (i % 10), glm::vec3(0.6f, 0.5f, 0.4f));
        }
        ground.Draw(shader, viewMatrix); 
        if (type == RENDER_ALL) {
            skyBox.Draw(skyboxShader, viewMatrix, projectionMatrix);
        }
    }
    void World::RenderMesh(gps::Model3D &mesh, gps::Shader& shader, glm::mat4 view, glm::mat4 projection, 
                          glm::vec3 position, float rotationAngle, float scale, glm::vec3 colorOverride) {
       RenderMesh(mesh, shader, view, projection, position, rotationAngle, glm::vec3(scale), colorOverride);
    }
    void World::RenderMesh(gps::Model3D &mesh, gps::Shader& shader, glm::mat4 view, glm::mat4 projection, 
                          glm::vec3 position, float rotationAngle, glm::vec3 scaleVector, glm::vec3 colorOverride) {
        shader.useShaderProgram();
        GLint modelLoc = glGetUniformLocation(shader.shaderProgram, "model");
        GLint normalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0, 1, 0));
        model = glm::scale(model, scaleVector);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        if(normalMatrixLoc >= 0) {
            glm::mat3 normMat = glm::mat3(glm::inverseTranspose(view * model));
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normMat));
        }
        for(size_t i=0; i<mesh.meshes.size(); ++i) {
            glm::vec3 originalKd = mesh.meshes[i].Kd;
            if (colorOverride != glm::vec3(1.0f)) {
                 mesh.meshes[i].Kd = colorOverride;
            }
            mesh.meshes[i].Draw(shader);
            mesh.meshes[i].Kd = originalKd; 
        }
    }
    void World::RenderMesh(gps::Model3D &mesh, gps::Shader& shader, glm::mat4 view, glm::mat4 projection, 
                   glm::vec3 position, glm::vec3 direction, glm::vec3 scaleVector, glm::vec3 colorOverride) {
        shader.useShaderProgram();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        glm::vec3 up = glm::vec3(0, 1, 0);
        if (glm::abs(glm::dot(glm::normalize(direction), up)) > 0.95f) up = glm::vec3(1, 0, 0);
        glm::mat4 rotation = glm::inverse(glm::lookAt(glm::vec3(0.0f), direction, up));
        model = model * rotation;
        model = glm::scale(model, scaleVector);
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        GLint normalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
        if(normalMatrixLoc >= 0) {
            glm::mat3 normMat = glm::mat3(glm::inverseTranspose(view * model));
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normMat));
        }
        for(size_t i=0; i<mesh.meshes.size(); ++i) {
             glm::vec3 originalKd = mesh.meshes[i].Kd;
             if (colorOverride != glm::vec3(1.0f)) mesh.meshes[i].Kd = colorOverride;
             mesh.meshes[i].Draw(shader);
             mesh.meshes[i].Kd = originalKd;
        }
}
}