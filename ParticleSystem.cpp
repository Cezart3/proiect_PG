#include "ParticleSystem.hpp"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>

namespace gps {

    ParticleSystem::ParticleSystem() {
    }

    void ParticleSystem::Init(int count, glm::vec3 spawnCenter, glm::vec3 range) {
        particleCount = count;
        spawnRange = range;
        
        particles.resize(count);
        for(int i=0; i<count; ++i) {
            float x = ((rand() % 1000) / 500.0f - 1.0f) * range.x; // -range to +range
            float y = ((rand() % 1000) / 1000.0f) * range.y;       // 0 to range
            float z = ((rand() % 1000) / 500.0f - 1.0f) * range.z; // -range to +range
            
            particles[i].position = spawnCenter + glm::vec3(x, y, z);
            particles[i].speed = 10.0f + (rand() % 100) / 10.0f; // 10 to 20 units/sec
        }

        shader.loadShader("shaders/particle.vert", "shaders/particle.frag");

        // Init Buffers (Dynamic Draw)
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // Initial empty manual upload or just reserve?
        // Let's reserve.
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(glm::vec3), NULL, GL_DYNAMIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        
        glBindVertexArray(0);
    }

    void ParticleSystem::Update(float delta, glm::vec3 centerPos) {
        // Simple Wind Vector
        glm::vec3 wind = glm::vec3(5.0f, 0.0f, 2.0f); 
        
        // Update positions
        for(int i=0; i<particleCount; ++i) {
            particles[i].position.y -= particles[i].speed * delta;
            particles[i].position.x += wind.x * delta; // Wind X
            particles[i].position.z += wind.z * delta; // Wind Z
            
            // Check bounds relative to centerPos
            // If too far below or too far from center XZ, respawn at top
            
            // Simple logic: If below 0 (ground) respawn at Top relative to player
            // This creates an "infinite rain" effect around the player
            float resetHeight = centerPos.y + spawnRange.y / 1.5f; 
            if(particles[i].position.y < 0.0f) {
                float x = ((rand() % 1000) / 500.0f - 1.0f) * spawnRange.x; 
                float z = ((rand() % 1000) / 500.0f - 1.0f) * spawnRange.z;
                
                particles[i].position = centerPos + glm::vec3(x, spawnRange.y/2.0f, z);
                particles[i].position.y = resetHeight + (rand() % 20); // Random offset
            }
        }
        
        // Update GPU Buffer
        // We pack positions into a simple vector for upload
        std::vector<glm::vec3> positions;
        positions.reserve(particleCount * 2); 
        
        for(const auto& p : particles) {
            // Draw as LINE (Streak)
            // Top point
            positions.push_back(p.position);
            // Bottom point (Streak length depends on speed + wind slant)
            // Slant the rain based on wind
            glm::vec3 tailoffset = glm::vec3(-wind.x * 0.1f, 0.5f, -wind.z * 0.1f);
            positions.push_back(p.position - tailoffset); 
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // Consider mapping/unmapping for speed, but BufferSubData is fine for < 5000 lines
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_DYNAMIC_DRAW);
    }

    void ParticleSystem::Draw(glm::mat4 view, glm::mat4 projection) {
        shader.useShaderProgram();
        
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

        glBindVertexArray(VAO);
        // Draw Lines (2 vertices per particle)
        glDrawArrays(GL_LINES, 0, particleCount * 2);
        glBindVertexArray(0);
    }

}
