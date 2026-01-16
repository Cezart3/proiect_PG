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
        enum RenderType {
            RENDER_ALL,
            RENDER_SHADOWS
        };

        World();
        
        void Init();
        // Update dynamic elements (asteroids, etc.)
        void Update(float delta);
        void Draw(gps::Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, RenderType type = RENDER_ALL);  
        
        // true if collision detected
        // true if collision detected
        bool CheckCollision(glm::vec3 position, float radius);

        // Shooting
        void FireBullet(glm::vec3 position, glm::vec3 direction);

    private:
        // Helper to render static objects with transforms
        void RenderMesh(gps::Model3D &mesh, gps::Shader& shader, glm::mat4 view, glm::mat4 projection, 
                       glm::vec3 position, float rotationAngle, float scale, glm::vec3 colorOverride = glm::vec3(1.0f));
                       
        // Overload for Non-Uniform Scaling (e.g. Tall Spires)
        void RenderMesh(gps::Model3D &mesh, gps::Shader& shader, glm::mat4 view, glm::mat4 projection, 
                       glm::vec3 position, float rotationAngle, glm::vec3 scaleVector, glm::vec3 colorOverride = glm::vec3(1.0f));

        gps::SkyBox skyBox;
        gps::Shader skyboxShader;
        
        gps::Ground ground;
        
        // Models
        gps::Model3D rock;
        gps::Model3D crater;
        
        // Scene Objects
        // Obstacle struct is defined outside the class, but we use it here.
        std::vector<Obstacle> obstacles;
        std::vector<Obstacle> dynamicObstacles; // For moving asteroids
        
        // Procedural Storage
        std::vector<glm::vec3> asteroidPositions; // Cached positions for rendering
        std::vector<glm::vec3> spirePositions;
    struct BuildingInstance {
        glm::vec3 position;
        glm::vec3 scale;
        float rotation;
        glm::vec3 color; // Tint
        int type; // 0=Hangar, 1=Tower1, 2=Tower2
        int health; // Health Points
    };

    struct AlienInstance {
        glm::vec3 position;
        int type; // 0=Old, 1=New
        int health; // Health Points
    };

    struct Bullet {
        glm::vec3 position;
        glm::vec3 velocity;
        float life;
    };
        
    // Unified City Storage
    std::vector<BuildingInstance> cityBuildings;
    std::vector<AlienInstance> alienInstances; 
    std::vector<Bullet> bullets;

    gps::Model3D building;
    gps::Model3D alien;
    gps::Model3D sun;
    gps::Model3D tower1;
    gps::Model3D tower2;
    gps::Model3D newAlien;
    }; // Close class World

} // Close namespace gps

#endif /* World_hpp */
