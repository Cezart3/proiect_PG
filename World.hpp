#ifndef World_hpp
#define World_hpp

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

#include "Model3D.hpp"
#include "Shader.hpp"
#include "SkyBox.hpp"
#include "Ground.hpp"

namespace gps {

    struct Obstacle {
        glm::vec3 position;
        float radius;
    };

    struct PointLight {
        glm::vec3 position;
        glm::vec3 color;
        float constant;
        float linear;
        float quadratic;
    };

    struct SpotLight {
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 color;
        float cutOff;
        float outerCutOff;
        float constant;
        float linear;
        float quadratic;
        int active; 
    };

    class World {
    public:
        World();
        
        void Init();
        void Draw(gps::Shader& basicShader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
        
        // true if collision detected
        bool CheckCollision(glm::vec3 position, float radius);

    private:
        gps::SkyBox skyBox;
        gps::Shader skyboxShader;
        gps::Ground ground;
        
        gps::Model3D rock;
        gps::Model3D crater;
        
        std::vector<Obstacle> obstacles;
        
        // Helper to render static objects with transforms
        void RenderMesh(gps::Model3D &mesh, gps::Shader& shader, glm::mat4 view, glm::mat4 projection, 
                       glm::vec3 position, float rotationAngle, float scale, glm::vec3 colorOverride);
    };

}

#endif /* World_hpp */
