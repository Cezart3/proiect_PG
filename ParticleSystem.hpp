#ifndef ParticleSystem_hpp
#define ParticleSystem_hpp
#include <vector>
#include <glm/glm.hpp>
#include "Shader.hpp"
namespace gps {
    struct Particle {
        glm::vec3 position;
        float speed;
    };
    class ParticleSystem {
    public:
        ParticleSystem();
        void Init(int count, glm::vec3 spawnCenter, glm::vec3 spawnRange);
        void Update(float delta, glm::vec3 centerPos);
        void Draw(glm::mat4 view, glm::mat4 projection);
    private:
        std::vector<Particle> particles;
        int particleCount;
        glm::vec3 spawnRange;
        GLuint VAO, VBO;
        gps::Shader shader;
    };
}
#endif