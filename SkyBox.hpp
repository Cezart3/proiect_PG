#ifndef SkyBox_hpp
#define SkyBox_hpp
#include <vector>
#include <string>
#include <iostream>
#if defined (__APPLE__)
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.hpp"
namespace gps {
    class SkyBox {
    public:
        SkyBox();
        void Load(std::vector<std::string> cubeMapFaces);
        void Draw(gps::Shader shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
    private:
        GLuint skyboxVAO, skyboxVBO;
        GLuint textureID;
        void InitSkyBox();
        GLuint LoadSkyBoxTextures(std::vector<std::string> cubeMapFaces);
    };
}
#endif  