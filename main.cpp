#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Vertex and fragment shader sources
const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTex;
out vec2 TexCoord;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    TexCoord = aTex;
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform vec3 uColor;
uniform sampler2D uTex;
uniform bool useTex;
void main() {
    if(useTex)
        FragColor = texture(uTex, TexCoord);
    else
        FragColor = vec4(uColor, 1.0);
}
)";

// Simple vector struct
struct Vec3 {
    float x, y, z;
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
};

struct Enemy {
    Vec3 pos;
    bool alive = true;
};

float yaw = -90.0f, pitch = 0.0f;
glm::vec3 camPos = glm::vec3(0, 1.6f, 5);
glm::vec3 camFront = glm::vec3(0,0,-1);
glm::vec3 camUp = glm::vec3(0,1,0);
float lastX = 400, lastY = 300;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float camYVelocity = 0.0f;
bool isJumping = false;

std::vector<Enemy> enemies;

// Cube vertex data (centered at origin, size 1)
float cubeVertices[] = {
    // positions
    // Front face
    -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
    // Back face
    -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,   0.5f,-0.5f,-0.5f,
    // Left face
    -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  -0.5f, 0.5f,-0.5f,
    // Right face
     0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,   0.5f,-0.5f, 0.5f,
    // Top face
    -0.5f, 0.5f,-0.5f, -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,   0.5f, 0.5f,-0.5f,
    // Bottom face
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  -0.5f,-0.5f, 0.5f
};
unsigned int cubeIndices[] = {
    0,1,2, 2,3,0,      // Front
    4,5,6, 6,7,4,      // Back
    8,9,10, 10,11,8,   // Left
    12,13,14, 14,15,12,// Right
    16,17,18, 18,19,16,// Top
    20,21,22, 22,23,20 // Bottom
};

// Floor quad (XZ plane)
float floorVertices[] = {
    // positions         // texcoords
    -50.0f, 0.0f, -50.0f,  0.0f, 0.0f,
     50.0f, 0.0f, -50.0f, 10.0f, 0.0f,
     50.0f, 0.0f,  50.0f, 10.0f,10.0f,
    -50.0f, 0.0f,  50.0f,  0.0f,10.0f
};
unsigned int floorIndices[] = {
    0,1,2, 2,3,0
};

// Gun quad (2D, NDC space)
float gunVertices[] = {
    -0.08f, -0.15f, 0.0f,
     0.08f, -0.15f, 0.0f,
     0.08f, -0.05f, 0.0f,
    -0.08f, -0.05f, 0.0f
};
unsigned int gunIndices[] = {
    0,1,2, 2,3,0
};

GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        std::cerr << "Shader compile error: " << info << std::endl;
    }
    return shader;
}

GLuint createShaderProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 dir;
    dir.x = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
    dir.y = sinf(glm::radians(pitch));
    dir.z = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));
    camFront = glm::normalize(dir);
}

void process_input(GLFWwindow* window) {
    float speed = 5.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += camFront * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= camFront * speed;
    glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += right * speed;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJumping) {
        camYVelocity = 6.0f; // jump strength
        isJumping = true;
    }

}
bool rayIntersectsAABB(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDir,
    const glm::vec3& boxCenter,
    float boxHalfSize,
    float& tHit)
{
    glm::vec3 minB = boxCenter - glm::vec3(boxHalfSize);
    glm::vec3 maxB = boxCenter + glm::vec3(boxHalfSize);
    float tmin = (minB.x - rayOrigin.x) / rayDir.x;
    float tmax = (maxB.x - rayOrigin.x) / rayDir.x;
    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (minB.y - rayOrigin.y) / rayDir.y;
    float tymax = (maxB.y - rayOrigin.y) / rayDir.y;
    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;

    float tzmin = (minB.z - rayOrigin.z) / rayDir.z;
    float tzmax = (maxB.z - rayOrigin.z) / rayDir.z;
    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin) tmin = tzmin;
    if (tzmax < tmax) tmax = tzmax;

    tHit = tmin;
    return tmax > 0;
}

// Replace your shoot() function with this:
void shoot() {
    float closestT = 1e9f;
    Enemy* hitEnemy = nullptr;
    glm::vec3 rayOrigin = camPos;
    glm::vec3 rayDir = glm::normalize(camFront);

    for (auto& e : enemies) {
        if (!e.alive) continue;
        float tHit;
        // Assuming cube size 1.0, so half size is 0.5
        if (rayIntersectsAABB(rayOrigin, rayDir, glm::vec3(e.pos.x, e.pos.y, e.pos.z), 0.5f, tHit)) {
            if (tHit > 0.0f && tHit < closestT && tHit < 100.0f) { // 100.0f = max shooting distance
                closestT = tHit;
                hitEnemy = &e;
            }
        }
    }
    if (hitEnemy) {
        hitEnemy->alive = false;
        std::cout << "Enemy hit!\n";
    }
}
GLuint loadTexture(const char* path) {
    int w, h, ch;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
    if (!data) { std::cerr << "Failed to load texture: " << path << std::endl; return 0; }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, ch == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return tex;
}

void drawObject(GLuint vao, GLuint shader, int indicesCount, glm::mat4 mvp, glm::vec3 color, GLuint tex = 0) {
    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "uMVP"), 1, GL_FALSE, &mvp[0][0]);
    glUniform3fv(glGetUniformLocation(shader, "uColor"), 1, &color[0]);
    glUniform1i(glGetUniformLocation(shader, "useTex"), tex ? 1 : 0);
    if (tex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(glGetUniformLocation(shader, "uTex"), 0);
    }
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    if (tex) glBindTexture(GL_TEXTURE_2D, 0);
}
bool prevMousePressed = false;
int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Simple FPS Modern", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);

    // Compile shaders
    GLuint shader = createShaderProgram();

    // Cube VAO/VBO/EBO
    GLuint cubeVAO, cubeVBO, cubeEBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Floor VAO/VBO/EBO
    GLuint floorVAO, floorVBO, floorEBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glGenBuffers(1, &floorEBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Gun VAO/VBO/EBO (drawn in NDC)
    GLuint gunVAO, gunVBO, gunEBO;
    glGenVertexArrays(1, &gunVAO);
    glGenBuffers(1, &gunVBO);
    glGenBuffers(1, &gunEBO);
    glBindVertexArray(gunVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gunVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gunVertices), gunVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gunEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gunIndices), gunIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    GLuint floorTexture = loadTexture("floor.jpg"); // Place a floor.jpg in your project folder

    enemies.push_back({{0,1, -5}});
    enemies.push_back({{2,1, -8}});
    enemies.push_back({{-3,1, -6}});

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        process_input(window);

        const float gravity = -15.0f;
        float groundY = 1.6f; // standing height

        if (isJumping) {
            camYVelocity += gravity * deltaTime;
            camPos.y += camYVelocity * deltaTime;
            if (camPos.y <= groundY) {
                camPos.y = groundY;
                camYVelocity = 0.0f;
                isJumping = false;
            }
        }

        bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (mousePressed && !prevMousePressed) {
            // std::cout << "Mouse clicked, shooting...\n";
            shoot();
        }
        prevMousePressed = mousePressed;

        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        glm::mat4 projection = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

        // Draw floor
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 mvp = projection * view * model;
        drawObject(floorVAO, shader, 6, mvp, glm::vec3(0.3f, 0.7f, 0.3f), floorTexture);

        // Draw enemies
        for (auto& e : enemies) {
            if (!e.alive) continue;
            model = glm::translate(glm::mat4(1.0f), glm::vec3(e.pos.x, e.pos.y, e.pos.z));
            mvp = projection * view * model;
            drawObject(cubeVAO, shader, 36, mvp, glm::vec3(1,0,0));
        }

        // // Draw gun (in NDC, after disabling depth)
        // glDisable(GL_DEPTH_TEST);
        // glm::mat4 gunModel = glm::mat4(1.0f);
        // glm::mat4 gunProj = glm::mat4(1.0f); // NDC
        // glm::mat4 gunMVP = gunProj * gunModel;
        // drawObject(gunVAO, shader, 6, gunMVP, glm::vec3(0.2f, 0.2f, 0.2f));
        // glEnable(GL_DEPTH_TEST);

        // Draw a hand with gun (bigger gun quad, offset to lower right)
        glDisable(GL_DEPTH_TEST);
        glm::mat4 handModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.3f, -0.3f, 0.0f)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(2.8f, 2.0f, 1.0f));
        glm::mat4 handMVP = handModel; // NDC
        drawObject(gunVAO, shader, 6, handMVP, glm::vec3(0.4f, 0.3f, 0.2f)); // brownish hand/gun
        glEnable(GL_DEPTH_TEST);

        // Draw a crosshair in the center of the screen
        float crosshairVertices[] = {
            // x, y
            -0.01f,  0.0f, 0.0f,
             0.01f,  0.0f, 0.0f,
             0.0f, -0.01f, 0.0f,
             0.0f,  0.01f, 0.0f
        };
        GLuint crossVAO, crossVBO;
        glGenVertexArrays(1, &crossVAO);
        glGenBuffers(1, &crossVBO);
        glBindVertexArray(crossVAO);
        glBindBuffer(GL_ARRAY_BUFFER, crossVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        // In main loop, after drawing hand/gun:
        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader, "uMVP"), 1, GL_FALSE, &glm::mat4(1.0f)[0][0]);
        glUniform3fv(glGetUniformLocation(shader, "uColor"), 1, &glm::vec3(1,1,1)[0]);
        glUniform1i(glGetUniformLocation(shader, "useTex"), 0);
        glBindVertexArray(crossVAO);
        glDrawArrays(GL_LINES, 0, 2);
        glDrawArrays(GL_LINES, 2, 2);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteBuffers(1, &floorEBO);
    glDeleteVertexArrays(1, &gunVAO);
    glDeleteBuffers(1, &gunVBO);
    glDeleteBuffers(1, &gunEBO);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
