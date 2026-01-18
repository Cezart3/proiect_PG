#include "Ground.hpp"
#include "stb_image.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp> 
#include <iostream>
namespace gps {
    Ground::Ground() {
    }
    void Ground::Load(std::string texturePath) {
        InitGround();
        textureID = LoadTexture(texturePath);
    }
    void Ground::Draw(gps::Shader& shader, glm::mat4 viewMatrix) {
        shader.useShaderProgram();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(2000.0f, 1.0f, 2000.0f)); 
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * model)); 
        glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glm::vec3 Ka = glm::vec3(0.2f); 
        glm::vec3 Kd = glm::vec3(0.8f); 
        glm::vec3 Ks = glm::vec3(0.0f); 
        glUniform3fv(glGetUniformLocation(shader.shaderProgram, "Ka"), 1, glm::value_ptr(Ka));
        glUniform3fv(glGetUniformLocation(shader.shaderProgram, "Kd"), 1, glm::value_ptr(Kd));
        glUniform3fv(glGetUniformLocation(shader.shaderProgram, "Ks"), 1, glm::value_ptr(Ks));
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "hasTexture"), 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "diffuseTexture"), 0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBindVertexArray(groundVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    void Ground::InitGround() {
        float vertices[] = {
             1.0f, 0.0f,  1.0f,   0.0f, 1.0f, 0.0f,  50.0f, 0.0f, 
             1.0f, 0.0f, -1.0f,   0.0f, 1.0f, 0.0f,  50.0f, 50.0f, 
            -1.0f, 0.0f, -1.0f,   0.0f, 1.0f, 0.0f,  0.0f,  50.0f, 
            -1.0f, 0.0f,  1.0f,   0.0f, 1.0f, 0.0f,  0.0f,  0.0f 
        };
        unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3
        };
        glGenVertexArrays(1, &groundVAO);
        glGenBuffers(1, &groundVBO);
        glGenBuffers(1, &groundEBO);
        glBindVertexArray(groundVAO);
        glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
    GLuint Ground::LoadTexture(std::string path) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        int width, height, nrComponents;
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            stbi_image_free(data);
        }
        else {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }
        return textureID;
    }
}