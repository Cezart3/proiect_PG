#ifndef Ground_hpp
#define Ground_hpp
#include <vector>
#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.hpp"
namespace gps {
    class Ground {
    public:
        Ground();
        void Load(std::string texturePath);
        void Draw(gps::Shader& shader, glm::mat4 viewMatrix);
    private:
        GLuint groundVAO, groundVBO, groundEBO;
        GLuint textureID;
        void InitGround();
        GLuint LoadTexture(std::string path);
    };
}
#endif  