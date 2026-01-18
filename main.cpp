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
#include "Drone.hpp" 
#include "World.hpp" 
#include "ParticleSystem.hpp" 
#include <iostream>
gps::Window myWindow;
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::vec3 lightDir;
glm::vec3 lightColor;
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
gps::Camera myCamera(
    glm::vec3(0.0f, 2.0f, 5.5f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));
GLboolean pressedKeys[1024];
gps::Drone myPlayerDrone;
gps::World myWorld;
gps::ParticleSystem rainSystem;
bool rainActive = false;
float lightAngle = 0.0f;
gps::Model3D fleetDrone; 
gps::Shader myBasicShader; 
gps::Shader depthMapShader;
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
}
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glm::vec4 viewport = glm::vec4(0, 0, width, height);
        glm::vec3 winCoordsNear = glm::vec3(xpos, height - ypos, 0.0f);
        glm::vec3 winCoordsFar = glm::vec3(xpos, height - ypos, 1.0f);
        glm::vec3 nearPoint = glm::unProject(winCoordsNear, view, projection, viewport);
        glm::vec3 farPoint = glm::unProject(winCoordsFar, view, projection, viewport);
        glm::vec3 rayDir = glm::normalize(farPoint - nearPoint);
        myWorld.FireBullet(myPlayerDrone.GetPosition(), rayDir); 
        glm::vec3 targetPoint = nearPoint + rayDir * 1000.0f; 
        glm::vec3 fireDir = glm::normalize(targetPoint - myPlayerDrone.GetPosition());
        myWorld.FireBullet(myPlayerDrone.GetPosition(), fireDir);
    }
}
void processMovement(float delta) {
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
    if (pressedKeys[GLFW_KEY_J]) {
        lightAngle -= 1.0f;
    }
    if (pressedKeys[GLFW_KEY_L]) {
        lightAngle += 1.0f;
    }
    myCamera.updatePresentation(delta);
    if (myCamera.isPresentationActive()) return;
    myPlayerDrone.Update(delta, pressedKeys, myWorld);
    static bool pPressed = false;
    if (pressedKeys[GLFW_KEY_P] && !pPressed) {
        rainActive = !rainActive;
        pPressed = true;
    }
    if (!pressedKeys[GLFW_KEY_P]) pPressed = false;
    if (rainActive) {
        rainSystem.Update(delta, myPlayerDrone.GetPosition());
    }
    static bool lPressed = false;
    if (pressedKeys[GLFW_KEY_L] && !lPressed) {
        static bool flashlightOn = true;
        flashlightOn = !flashlightOn;
        myBasicShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.active"), flashlightOn ? 1 : 0);
        lPressed = true;
    }
    if (!pressedKeys[GLFW_KEY_L]) lPressed = false;
    static bool cPressed = false;
    static bool fogEnabled = true; 
    if (pressedKeys[GLFW_KEY_C] && !cPressed) {
        fogEnabled = !fogEnabled;
        myBasicShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "fogActive"), fogEnabled ? 1 : 0);
        cPressed = true;
        std::cout << "Fog Toggled: " << (fogEnabled ? "ON" : "OFF") << std::endl;
    }
    if (!pressedKeys[GLFW_KEY_C]) cPressed = false;
    if (pressedKeys[GLFW_KEY_1]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        myBasicShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "isFlat"), 0);
    }
    if (pressedKeys[GLFW_KEY_2]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        myBasicShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "isFlat"), 0);
    }
    if (pressedKeys[GLFW_KEY_3]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        myBasicShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "isFlat"), 1);
    }
    if (pressedKeys[GLFW_KEY_4]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }
}
void updateCamera() {
    if (myCamera.isPresentationActive()) {
        view = myCamera.getViewMatrix();
        return; 
    }
    glm::vec3 dronePos = myPlayerDrone.GetPosition();
    glm::vec3 forward = myPlayerDrone.GetForward();
    glm::vec3 up = myPlayerDrone.GetUp(); 
    glm::vec3 cameraOffset = -forward * 30.0f + up * 15.0f; 
    glm::vec3 targetPos = dronePos + cameraOffset;
    glm::vec3 currentPos = myCamera.getPosition();
    glm::vec3 newPos = glm::mix(currentPos, targetPos, 0.1f);
    myCamera.setPosition(newPos);
    myCamera.setTarget(dronePos + forward * 10.0f); 
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glm::vec3 lightPos = dronePos + forward * 2.0f; 
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
    glfwSetMouseButtonCallback(myWindow.getWindow(), mouseButtonCallback);
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
    myPlayerDrone.Load("models/nava_noua/13897_Sci-Fi_Fighter_Ship_v1_l1.obj");
    fleetDrone.LoadModel("models/kenney_space-kit/Models/OBJ format/craft_speederA.obj");
    myWorld.Init();
    rainSystem.Init(3000, glm::vec3(0, 50, 0), glm::vec3(400.0f, 100.0f, 400.0f));
}
void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
    if (depthMapShader.shaderProgram == 0) {
        std::cerr << "Depth Map Shader failed to load!" << std::endl;
    }
}
void initFBO() {
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
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
	lightDir = glm::vec3(0.0f, 10.0f, 10.0f); 
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightColor"), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.constant"), 1.0f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.linear"), 0.009f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.quadratic"), 0.0032f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.outerCutOff"), glm::cos(glm::radians(17.5f)));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.color"), 1, glm::value_ptr(glm::vec3(1.0f, 0.9f, 0.8f))); 
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.active"), 1); 
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "fogActive"), 1); 
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "isFlat"), 0); 
}
void drawObjects(gps::Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
    shader.useShaderProgram();
    myPlayerDrone.Draw(shader, viewMatrix); 
}
void renderFleetMember(gps::Shader& shader, glm::vec3 position, float rotationAngle, glm::vec3 colorOverride, glm::mat4 viewMatrix) {
    shader.useShaderProgram();
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0, 1, 0));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
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
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
    depthMapShader.useShaderProgram();
    glDisable(GL_CULL_FACE);
    depthMapShader.useShaderProgram();
    glm::vec3 dronePos = myPlayerDrone.GetPosition();
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.5f)); 
    float orthoSize = 300.0f; 
    glm::vec3 lightPos = dronePos + lightDir * orthoSize; 
    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 2000.0f); 
    glm::mat4 lightView = glm::lookAt(lightPos, dronePos, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    myPlayerDrone.Draw(depthMapShader, lightView); 
    renderFleetMember(depthMapShader, glm::vec3(30.0f, 10.0f, 30.0f), 45.0f, glm::vec3(1.0f), lightView);
    renderFleetMember(depthMapShader, glm::vec3(-50.0f, 20.0f, -40.0f), -30.0f, glm::vec3(1.0f), lightView);
    myWorld.Draw(depthMapShader, lightView, lightProjection, gps::World::RENDER_SHADOWS);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glm::mat4 lightRot = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0, 1, 0));
    glm::vec3 sunDir = glm::vec3(lightRot * glm::vec4(0.0f, 10.0f, 10.0f, 0.0f)); 
    sunDir = glm::normalize(sunDir);
    glm::vec3 sunDirEye = glm::vec3(view * glm::vec4(sunDir, 0.0f));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(sunDirEye));
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);
    myPlayerDrone.Draw(myBasicShader, view); 
    if (myPlayerDrone.GetBoosting()) {
         glm::vec3 dronePos = myPlayerDrone.GetPosition();
         glm::vec3 droneFwd = myPlayerDrone.GetForward();
         glm::vec3 droneRight = glm::normalize(glm::cross(droneFwd, glm::vec3(0, 1, 0))); 
         float offsetBack = 6.0f;
         float offsetSide = 0.5f;
         glm::vec3 exhaustLeft = dronePos - droneFwd * offsetBack - droneRight * offsetSide;
         glm::vec3 exhaustRight = dronePos - droneFwd * offsetBack + droneRight * offsetSide;
         glm::vec3 coreScale = glm::vec3(0.05f, 0.05f, 8.0f);
         glm::vec3 coreColor = glm::vec3(2.0f, 2.0f, 2.0f); 
         glm::vec3 haloScale = glm::vec3(0.15f, 0.15f, 10.0f);
         glm::vec3 haloColor = glm::vec3(0.1f, 0.5f, 1.0f); 
         myWorld.RenderMesh(myWorld.nitroModel, myBasicShader, view, projection, exhaustLeft, -droneFwd, coreScale, coreColor);
         myWorld.RenderMesh(myWorld.nitroModel, myBasicShader, view, projection, exhaustLeft, -droneFwd, haloScale, haloColor);
         myWorld.RenderMesh(myWorld.nitroModel, myBasicShader, view, projection, exhaustRight, -droneFwd, coreScale, coreColor);
         myWorld.RenderMesh(myWorld.nitroModel, myBasicShader, view, projection, exhaustRight, -droneFwd, haloScale, haloColor);
    }
    renderFleetMember(myBasicShader, glm::vec3(30.0f, 10.0f, 30.0f), 45.0f, glm::vec3(0.0f, 1.0f, 1.0f), view);
    renderFleetMember(myBasicShader, glm::vec3(-50.0f, 20.0f, -40.0f), -30.0f, glm::vec3(1.0f, 0.0f, 0.0f), view);
    myWorld.Draw(myBasicShader, view, projection, gps::World::RENDER_ALL);
    static double lastFireTime = 0.0;
    GLFWwindow* window = myWindow.getWindow();
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double currentTime = glfwGetTime();
        if (currentTime - lastFireTime > 0.15) { 
             int width, height;
             glfwGetWindowSize(window, &width, &height);
             double xpos, ypos;
             glfwGetCursorPos(window, &xpos, &ypos);
             glm::vec3 winCoords = glm::vec3(xpos, height - ypos, 0.0f);
             glm::vec4 viewport = glm::vec4(0, 0, width, height);
             glm::vec3 nearPt = glm::unProject(glm::vec3(winCoords.x, winCoords.y, 0.0f), view, projection, viewport);
             glm::vec3 farPt = glm::unProject(glm::vec3(winCoords.x, winCoords.y, 1.0f), view, projection, viewport);
             glm::vec3 direction = glm::normalize(farPt - nearPt);
             myWorld.FireBullet(myPlayerDrone.GetPosition(), direction);
             lastFireTime = currentTime;
        }
    }
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
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        double currentTimeStamp = glfwGetTime();
        float delta = (float)(currentTimeStamp - lastTimeStamp);
        lastTimeStamp = currentTimeStamp;
        processMovement(delta);
        myWorld.Update(delta); 
        updateCamera();
	    renderScene();
		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());
		glCheckError();
	}
	cleanup();
    return EXIT_SUCCESS;
}