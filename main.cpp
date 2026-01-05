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
#include "ParticleSystem.hpp" // Rain

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
gps::ParticleSystem rainSystem;
bool rainActive = false;
float lightAngle = 0.0f;

// Additional fleet drone (keeping it here for now as simple prop, or move to World later?)
// Let's keep one simple prop usage here to show how to mix them, or move to World?
// User asked for "all logic in main" earlier, now modular. 
// Fleet drones are part of the "World" technically. 
// But let's leave them here for now to avoid over-engineering World yet.
gps::Model3D fleetDrone; 

// shaders
// shaders
gps::Shader myBasicShader; 
gps::Shader depthMapShader;

// Shadow Map
GLuint shadowMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 4096;



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
    // Toggle Presentation (I key) - Allowed anytime
    static bool iPressed = false;
    if (pressedKeys[GLFW_KEY_I] && !iPressed) {
        if (myCamera.isPresentationActive()) {
            myCamera.stopPresentation();
        } else {
            myCamera.startPresentation();
        }
        iPressed = true;
    }
    if (!pressedKeys[GLFW_KEY_I]) iPressed = false;

    // Light Control
    if (pressedKeys[GLFW_KEY_J]) {
        lightAngle -= 1.0f;
    }
    if (pressedKeys[GLFW_KEY_L]) {
        lightAngle += 1.0f;
    }

    // Update Presentation Camera (must happen even if presentation is active)
    myCamera.updatePresentation(delta);

    // If Presentation is ON, disable other controls
    if (myCamera.isPresentationActive()) return;

    // Delegate to Drone Class
    myPlayerDrone.Update(delta, pressedKeys, myWorld);
    
    // Toggle Rain (P key)
    static bool pPressed = false;
    if (pressedKeys[GLFW_KEY_P] && !pPressed) {
        rainActive = !rainActive;
        pPressed = true;
    }
    if (!pressedKeys[GLFW_KEY_P]) pPressed = false;
    
    if (rainActive) {
        rainSystem.Update(delta, myPlayerDrone.GetPosition());
    }
        
    // Toggle Flashlight (F key)
    static bool fPressed = false;
    if (pressedKeys[GLFW_KEY_F] && !fPressed) {
        static bool flashlightOn = true;
        flashlightOn = !flashlightOn;
        myBasicShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.active"), flashlightOn ? 1 : 0);
        fPressed = true;
    }
    if (!pressedKeys[GLFW_KEY_F]) fPressed = false;
    
    // Toggle Fog (C key)
    static bool cPressed = false;
    static bool fogEnabled = true; // Default ON
    if (pressedKeys[GLFW_KEY_C] && !cPressed) {
        fogEnabled = !fogEnabled;
        myBasicShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "fogActive"), fogEnabled ? 1 : 0);
        cPressed = true;
        std::cout << "Fog Toggled: " << (fogEnabled ? "ON" : "OFF") << std::endl;
    }
    if (!pressedKeys[GLFW_KEY_C]) cPressed = false;
    
    // Render Modes
    // 1: Wireframe
    if (pressedKeys[GLFW_KEY_1]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        myBasicShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "isFlat"), 0);
    }
    // 2: Smooth (Solid)
    if (pressedKeys[GLFW_KEY_2]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        myBasicShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "isFlat"), 0);
    }
    // 3: Polygonal (Solid + Flat Shading)
    if (pressedKeys[GLFW_KEY_3]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        myBasicShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "isFlat"), 1);
    }
    // 4: Point
    if (pressedKeys[GLFW_KEY_4]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }
}

void updateCamera() {
    // Override if in Presentation Mode
    if (myCamera.isPresentationActive()) {
        // Presentation Logic handles updates internally via myCamera.updatePresentation
        // But we MUST update the view matrix global!
        view = myCamera.getViewMatrix();
        return; 
    }

    // Standard 3rd Person Follow Logic
    glm::vec3 dronePos = myPlayerDrone.GetPosition();
    glm::vec3 forward = myPlayerDrone.GetForward();
    glm::vec3 up = myPlayerDrone.GetUp(); 
    
    // Desired Position
    glm::vec3 cameraOffset = -forward * 30.0f + up * 15.0f; 
    glm::vec3 targetPos = dronePos + cameraOffset;
    
    // Very simple smoothing (Lerp)
    glm::vec3 currentPos = myCamera.getPosition();
    glm::vec3 newPos = glm::mix(currentPos, targetPos, 0.1f);
    
    myCamera.setPosition(newPos);
    myCamera.setTarget(dronePos + forward * 10.0f); // Look slightly ahead of drone
    
    // Update the view matrix directly
    view = myCamera.getViewMatrix();
    
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
    
    // Init Rain (2000 drops, range 300x50x300 around player)
    rainSystem.Init(3000, glm::vec3(0, 50, 0), glm::vec3(400.0f, 100.0f, 400.0f));
}

void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
    
    // Check for shader errors
    if (depthMapShader.shaderProgram == 0) {
        std::cerr << "Depth Map Shader failed to load!" << std::endl;
    }
}

void initFBO() {
    // Generate and bind the framebuffer
    glGenFramebuffers(1, &shadowMapFBO);
    
    // Create depth texture
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    
    // Determine that FBO does not need any color data
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "fogActive"), 1); // Fog On by default
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "isFlat"), 0); // Smooth by default
}

// Helper to draw the scene geometry (used by both passes)
void drawObjects(gps::Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
    shader.useShaderProgram();
    
    // Set View/Proj (if shader needs them - depth shader technically only needs lightSpaceMatrix passed as 'projection' or similar, 
    // but we usually pass lightSpaceMatrix as a separate uniform. 
    // However, for BasicShader, we need view/proj.
    
    // Note: depthMap.vert uses 'lightSpaceMatrix' and 'model'. 
    // basic.vert uses 'view', 'projection', 'model'.
    
    // Uniforms MUST be set by the caller (renderScene) because they differ per pass.
    // This function only issues the Draw calls and sets Model matrix.
    
    // But wait, Drone.Draw and World.Draw set their own Model matrices internally.
    // So we just need to call them. 
    // They accept a shader.
    
    // Player
    myPlayerDrone.Draw(shader, viewMatrix); // Drone.Draw sets 'model' and 'normalMatrix'
    
    // Static Fleet (Manually set model here)
    // We need to refactor generic drawing or just duplicate calls?
    // Let's copy the calls.
    
    // Fleet Code from prev RenderScene replacement:
    // ... logic for fleet ...
    // renderFleetMember helper sets its own uniforms. 
    // We need to update renderFleetMember to take a shader!
}

// Updated Helper
void renderFleetMember(gps::Shader& shader, glm::vec3 position, float rotationAngle, glm::vec3 colorOverride, glm::mat4 viewMatrix) {
    shader.useShaderProgram();
    
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0, 1, 0));
    
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    
    // Only set normalMatrix if not depth shader (optimization, or check uniform loc)
    // Depth shader doesn't have normalMatrix.
    GLint normLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
    if(normLoc >= 0) {
        glm::mat3 normMat = glm::mat3(glm::inverseTranspose(viewMatrix * modelMatrix));
        glUniformMatrix3fv(normLoc, 1, GL_FALSE, glm::value_ptr(normMat));
    }

    for(size_t i=0; i<fleetDrone.meshes.size(); ++i) {
        if(colorOverride != glm::vec3(1.0f)) fleetDrone.meshes[i].Kd = colorOverride;
        fleetDrone.meshes[i].Draw(shader);
    }
}

void renderScene() {
    // 1. SHADOW PASS
    // Unbind shadow texture to prevent Feedback Loop error (Texture cannot be bound as sampler and FBO attachment simultaneously)
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);

    depthMapShader.useShaderProgram();
    
    // Disable Face Culling for Shadows (Double-sided shadows are safer/better)
    glDisable(GL_CULL_FACE);
    
    depthMapShader.useShaderProgram();
    
    // 1. Center the shadow map on the player
    // This ensures shadows follow the drone wherever it goes
    glm::vec3 dronePos = myPlayerDrone.GetPosition();
    
    // Light direction (fixed sun)
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.5f)); // Normalized sun direction
    
    // Position the "Light Camera" appropriately to cover the area around the drone
    // We move the light source along the lightDir relative to the drone
    float orthoSize = 300.0f; // View radius
    glm::vec3 lightPos = dronePos + lightDir * orthoSize; // INCREASED DISTANCE (was / 2.0f)
    
    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 2000.0f); // INCREASED FAR PLANE
    glm::mat4 lightView = glm::lookAt(lightPos, dronePos, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    
    // Update Global Light Direction for Lighting Pass too (keep consistent)
    // Note: In lighting pass we send Eye Space, so we'll recalculate there.
    // But conceptually, the sun is now "Infinite" direction.
    
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Draw Objects for Shadow Map
    // Drone
    // We need Drone::Draw to work with Depth Shader? 
    // Drone.Draw sets 'model', 'view'?, 'normalMatrix'.
    // Depth shader only has 'model' and 'lightSpaceMatrix'.
    // If we pass 'lightView' as 'viewMatrix' to Drone::Draw, it sets 'view' uniform. 
    // Depth shader doesn't have 'view' uniform, so it's ignored (GL_INVALID_OPERATION? No, just -1).
    // But Drone.Draw DOES NOT set 'lightSpaceMatrix'. 
    // WE NEED A GENERIC 'Draw(shader)' in Drone/World that sets MODEL matrix.
    
    // For now, let's just accept that Drone.Draw sets extra uniforms that might fail if loc is -1?
    // In OpenGL, setting uniform to -1 location is silently ignored. SAFE.
    // So we can reuse Drone::Draw(depthMapShader, lightView).
    
    myPlayerDrone.Draw(depthMapShader, lightView); 
    renderFleetMember(depthMapShader, glm::vec3(30.0f, 10.0f, 30.0f), 45.0f, glm::vec3(1.0f), lightView);
    renderFleetMember(depthMapShader, glm::vec3(-50.0f, 20.0f, -40.0f), -30.0f, glm::vec3(1.0f), lightView);
    myWorld.Draw(depthMapShader, lightView, lightProjection, gps::World::RENDER_SHADOWS);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // 2. LIGHTING PASS
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    myBasicShader.useShaderProgram();
    
    // Send View/Proj
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    // Send Light/Shadow Uniforms
    // Send Light/Shadow Uniforms
    // Dynamic Sun Direction (Rotate around Y axis based on lightAngle)
    glm::mat4 lightRot = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0, 1, 0));
    glm::vec3 sunDir = glm::vec3(lightRot * glm::vec4(0.0f, 10.0f, 10.0f, 0.0f)); // Start high noon-ish
    // Normalize
    sunDir = glm::normalize(sunDir);

    // Transform to Eye Space
    glm::vec3 sunDirEye = glm::vec3(view * glm::vec4(sunDir, 0.0f));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(sunDirEye));
    
    // Send LightSpaceMatrix to BasicShader for shadow calc
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
    // Bind Shadow Map
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);
    
    // Draw Objects
    myPlayerDrone.Draw(myBasicShader, view); 
    renderFleetMember(myBasicShader, glm::vec3(30.0f, 10.0f, 30.0f), 45.0f, glm::vec3(0.0f, 1.0f, 1.0f), view);
    renderFleetMember(myBasicShader, glm::vec3(-50.0f, 20.0f, -40.0f), -30.0f, glm::vec3(1.0f, 0.0f, 0.0f), view);
    myWorld.Draw(myBasicShader, view, projection, gps::World::RENDER_ALL);
    
    if (rainActive) {
        rainSystem.Draw(view, projection);
    }
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
    initFBO();
    setWindowCallbacks();

	glCheckError();
    
    double lastTimeStamp = glfwGetTime();

	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        double currentTimeStamp = glfwGetTime();
        float delta = (float)(currentTimeStamp - lastTimeStamp);
        lastTimeStamp = currentTimeStamp;

        processMovement(delta);
        myWorld.Update(delta); // Update dynamic environment
        updateCamera();

	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
