#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/matrix_inverse.hpp> 
#include <glm/gtc/type_ptr.hpp> 

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "Drone.hpp" // Modular Drone
#include "World.hpp" // Modular World

#include <iostream>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 2.0f, 5.5f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLboolean pressedKeys[1024];

// Game Objects
gps::Drone myPlayerDrone;
gps::World myWorld;

// Additional fleet drone (keeping it here for now as simple prop, or move to World later?)
// Let's keep one simple prop usage here to show how to mix them, or move to World?
// User asked for "all logic in main" earlier, now modular. 
// Fleet drones are part of the "World" technically. 
// But let's leave them here for now to avoid over-engineering World yet.
gps::Model3D fleetDrone; 

// shaders
gps::Shader myBasicShader; 
// World manages its own SkyboxShader, but shares BasicShader? Or manages internally?
// In World.cpp I used passed BasicShader.


GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
   // Optional mouse look
}

void processMovement(float delta) {
    // Delegate to Drone Class
    myPlayerDrone.Update(delta, pressedKeys, myWorld);
    
    // Toggle Flashlight (F key)
    static bool fPressed = false;
    if (pressedKeys[GLFW_KEY_F] && !fPressed) {
        static bool flashlightOn = true;
        flashlightOn = !flashlightOn;
        myBasicShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.active"), flashlightOn ? 1 : 0);
        fPressed = true;
    }
    if (!pressedKeys[GLFW_KEY_F]) {
        fPressed = false;
    }
}

void updateCamera() {
    float distance = 20.0f;
    float height = 6.0f;

    // Camera Strategy: Follow the drone stabilized
    glm::vec3 dronePos = myPlayerDrone.GetPosition();
    glm::vec3 forward = myPlayerDrone.GetForward(); 
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); 

    glm::vec3 cameraPos = dronePos - forward * distance + glm::vec3(0.0f, height, 0.0f);
    glm::vec3 lookAtTarget = dronePos + forward * 10.0f;

    view = glm::lookAt(cameraPos, lookAtTarget, up);
    
    // UPDATE SPOTLIGHT (Headlight)
    // Needs to be in Eye Space!
    myBasicShader.useShaderProgram();
    
    glm::vec3 lightPos = dronePos + forward * 2.0f; // Slightly ahead of drone
    glm::vec3 lightDir = forward;
    
    glm::vec3 lightPosEye = glm::vec3(view * glm::vec4(lightPos, 1.0f));
    glm::vec3 lightDirEye = glm::vec3(view * glm::vec4(lightDir, 0.0f));
    
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.position"), 1, glm::value_ptr(lightPosEye));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.direction"), 1, glm::value_ptr(lightDirEye));
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project - Modular World");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); 
	glDepthFunc(GL_LESS); 
	glEnable(GL_CULL_FACE); 
	glCullFace(GL_BACK); 
	glFrontFace(GL_CCW); 
}

void initModels() {
    myPlayerDrone.Load("models/kenney_space-kit/Models/OBJ format/craft_speederA.obj");
    fleetDrone.LoadModel("models/kenney_space-kit/Models/OBJ format/craft_speederA.obj");

    // Initialize World (Rocks, Skybox, Ground)
    myWorld.Init();
}

void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    // World manages SkyboxShader locally
}

void initUniforms() {
	myBasicShader.useShaderProgram();

	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 2000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	lightDir = glm::vec3(0.0f, 10.0f, 10.0f); // High Sun
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    // Send LightDir in Eye Space? 
    // Shader expects Eye Space currently? 
    // "vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));" inside calcDirLight? 
    // Wait, my new shader expects 'lightDir' to be Eye Space passed in?
    // Frag: "vec3 lightDirN = normalize(lightDir);" -> IMPLIES lightDir IS ALREADY EYE SPACE.
    // OLD shader did: "normalize(view * vec4(lightDir...))".
    // I NEED TO UPDATE main.cpp to send Eye Space Dir Light every frame (since View changes).
    
    // REMOVED static send here. Will send in RenderScene or UpdateCamera.
    // Initialize default colors
	glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightColor"), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));

    // Initialize SpotLight defaults
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.constant"), 1.0f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.linear"), 0.009f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.quadratic"), 0.0032f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.outerCutOff"), glm::cos(glm::radians(17.5f)));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.color"), 1, glm::value_ptr(glm::vec3(1.0f, 0.9f, 0.8f))); // Warm headlight
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.active"), 1); // On by default
}

// Temporary Helper for Fleet Drones (since they aren't in World class yet)
void renderFleetMember(glm::vec3 position, float rotationAngle, glm::vec3 colorOverride) {
    myBasicShader.useShaderProgram();
    
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0, 1, 0));
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glm::mat3 normMat = glm::mat3(glm::inverseTranspose(view * modelMatrix));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normMat));

    for(size_t i=0; i<fleetDrone.meshes.size(); ++i) {
        glm::vec3 originalKd = fleetDrone.meshes[i].Kd;
        float brightness = (originalKd.r + originalKd.g + originalKd.b) / 3.0f;
        
        if (brightness > 0.8f && colorOverride != glm::vec3(1.0f)) { 
             fleetDrone.meshes[i].Kd = colorOverride;
        }
        
        fleetDrone.meshes[i].Draw(myBasicShader);
        fleetDrone.meshes[i].Kd = originalKd; 
    }
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    
    // UPDATE SUN LIGHT (Eye Space)
    glm::vec3 sunDir = glm::vec3(100.0f, 100.0f, 100.0f); // Fixed Sun Position
    // Or Direction? LightDir usually points FROM light source? or TO?
    // Standard: SurfacePoint - LightPos = -LightDir.
    // Usually lightDir uniform is Direction TO Light or FROM Light?
    // Diffuse = max(dot(N, L), 0.0). L is vector TO light.
    // If we pass "lightDir", usually it's vector TO light.
    // Let's assume sunDir is vector TO sun.
    
    glm::vec3 sunDirEye = glm::vec3(view * glm::vec4(sunDir, 0.0f));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(sunDirEye));
    
	// Player
    myPlayerDrone.Draw(myBasicShader, view);

    // Fleet (Static Props)
    renderFleetMember(glm::vec3(30.0f, 10.0f, 30.0f), 45.0f, glm::vec3(0.0f, 1.0f, 1.0f));
    renderFleetMember(glm::vec3(-50.0f, 20.0f, -40.0f), -30.0f, glm::vec3(1.0f, 0.0f, 0.0f));

    // World (Ground, Skybox, Environment)
    myWorld.Draw(myBasicShader, view, projection);
}

void cleanup() {
    myWindow.Delete();
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
	initShaders();
	initUniforms();
    setWindowCallbacks();

	glCheckError();
    
    double lastTimeStamp = glfwGetTime();

	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        double currentTimeStamp = glfwGetTime();
        float delta = (float)(currentTimeStamp - lastTimeStamp);
        lastTimeStamp = currentTimeStamp;

        processMovement(delta);
        updateCamera();

	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
