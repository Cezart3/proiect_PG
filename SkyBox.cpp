#include "SkyBox.hpp"
#include "stb_image.h"
namespace gps {
    SkyBox::SkyBox() {
    }
    void SkyBox::Load(std::vector<std::string> cubeMapFaces) {
        InitSkyBox();
        textureID = LoadSkyBoxTextures(cubeMapFaces);
    }
    void SkyBox::Draw(gps::Shader shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
        shader.useShaderProgram();
        viewMatrix = glm::mat4(glm::mat3(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glDepthFunc(GL_LEQUAL);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "skybox"), 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
    }
    void SkyBox::InitSkyBox() {
        float skyboxVertices[] = {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }
    GLuint SkyBox::LoadSkyBoxTextures(std::vector<std::string> cubeMapFaces) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        int width, height, nrChannels;
        for (unsigned int i = 0; i < cubeMapFaces.size(); i++)
        {
            unsigned char *data = stbi_load(cubeMapFaces[i].c_str(), &width, &height, &nrChannels, 0);
            if (data)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                             0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
                );
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Cubemap texture failed to load at path: " << cubeMapFaces[i] << std::endl;
                stbi_image_free(data);
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        return textureID;
    }
}