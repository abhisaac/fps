#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <cmath>
#include <random>
#include <ctime>
#include <functional>
#include <cstring>
#include <algorithm>

// sound
#define NOMINMAX
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif
#pragma comment(lib, "winmm.lib")


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_easy_font.h"

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
const char* textVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;
out vec3 vColor;
uniform vec2 uOffset;
uniform float uScale;
void main() {
    gl_Position = vec4((aPos * uScale) + uOffset, 0.0, 1.0);
    vColor = aColor;
}
)";

const char* textFragmentShaderSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
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
    glm::vec3 velocity = glm::vec3(0);
    bool smashing = false;
    float smashTime = 0.0f;
};

// Maze parameters
const int MAZE_W = 15, MAZE_H = 15;
int maze[MAZE_H][MAZE_W] = {1}; // 0 = empty, 1 = wall
std::vector<glm::vec3> wallPositions;
std::vector<Enemy> enemies;

// Camera and player state
float yaw = -90.0f, pitch = 0.0f;
glm::vec3 camPos = glm::vec3(-6, 1.6f, -6);
glm::vec3 camFront = glm::vec3(0,0,-1);
glm::vec3 camUp = glm::vec3(0,1,0);
float lastX = 400, lastY = 300;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float camYVelocity = 0.0f;
bool isJumping = false;

struct Bullet {
    glm::vec3 pos;
    glm::vec3 dir;
    float speed;
    bool alive = true;
};
std::vector<Bullet> bullets;

struct GameParameters {
    float playerSpeed = 5.0f;
    float jumpStrength = 6.0f;
    float enemySpeed = 2.0f;
    float bulletSpeed = 18.0f;
    glm::vec3 enemyColor = glm::vec3(1,0,0);
    float wallHeight = 2.0f;
    bool showDebug = true;
} params;

float cubeVertices[] = {
    //  x      y      z      u     v
    // Front face
    -0.5f,-0.5f, 0.5f,  0.0f, 0.0f,
     0.5f,-0.5f, 0.5f,  1.0f, 0.0f,
     0.5f, 0.5f, 0.5f,  1.0f, 1.0f,
    -0.5f, 0.5f, 0.5f,  0.0f, 1.0f,
    // Back face
    -0.5f,-0.5f,-0.5f,  1.0f, 0.0f,
    -0.5f, 0.5f,-0.5f,  1.0f, 1.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 1.0f,
     0.5f,-0.5f,-0.5f,  0.0f, 0.0f,
    // Left face
    -0.5f,-0.5f,-0.5f,  0.0f, 0.0f,
    -0.5f,-0.5f, 0.5f,  1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f,  1.0f, 1.0f,
    -0.5f, 0.5f,-0.5f,  0.0f, 1.0f,
    // Right face
     0.5f,-0.5f,-0.5f,  1.0f, 0.0f,
     0.5f, 0.5f,-0.5f,  1.0f, 1.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 1.0f,
     0.5f,-0.5f, 0.5f,  0.0f, 0.0f,
    // Top face
    -0.5f, 0.5f,-0.5f,  0.0f, 1.0f,
    -0.5f, 0.5f, 0.5f,  0.0f, 0.0f,
     0.5f, 0.5f, 0.5f,  1.0f, 0.0f,
     0.5f, 0.5f,-0.5f,  1.0f, 1.0f,
    // Bottom face
    -0.5f,-0.5f,-0.5f,  1.0f, 1.0f,
     0.5f,-0.5f,-0.5f,  0.0f, 1.0f,
     0.5f,-0.5f, 0.5f,  0.0f, 0.0f,
    -0.5f,-0.5f, 0.5f,  1.0f, 0.0f
};
// 24 vertices, 5 floats each (position + texcoord)

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

void generateMaze() {
    // memset(maze, 1, sizeof(maze));
    for (int y = 0; y < MAZE_H; ++y)
        for (int x = 0; x < MAZE_W; ++x)
           maze[y][x] = 1;
        
    std::mt19937 rng((unsigned int)time(0));
    std::function<void(int,int)> carve = [&](int x, int y) {
        maze[y][x] = 0;
        std::vector<std::pair<int,int>> dirs = {{2,0},{-2,0},{0,2},{0,-2}};
        std::shuffle(dirs.begin(), dirs.end(), rng);
        for (auto [dx,dy] : dirs) {
            int nx = x+dx, ny = y+dy;
            // std::cout << nx << "," << ny << " -> ";
            if (nx>0 && nx<MAZE_W-1 && ny>0 && ny<MAZE_H-1 && maze[ny][nx]==1) {
                maze[y+dy/2][x+dx/2]=0; // Remove wall between
                carve(nx,ny);
            }
        }
    };
    carve(3,3);
    // std::cout << "Maze generated\n";
}

// --- Place walls as cubes in the scene ---
void buildWalls() {
    wallPositions.clear();
    for(int y=0;y<MAZE_H;++y) for(int x=0;x<MAZE_W;++x)
        if(maze[y][x]==1)
            wallPositions.push_back(glm::vec3(x*1.5f-10.5f, 1.0f, y*1.5f-10.5f)); // y=1.0f for center of tall wall
//     std::cout << "2" << std::endl;
}

// --- Place enemies in open cells ---
void spawnEnemies() {
    enemies.clear();
    std::vector<std::pair<int, int>> emptyCells;
    for (int y = 0; y < MAZE_H; ++y)
        for (int x = 0; x < MAZE_W; ++x)
            if (maze[y][x] == 0 && !(x == 1 && y == 1))
                emptyCells.emplace_back(x, y);

    std::mt19937 rng((unsigned int)time(0));
    std::shuffle(emptyCells.begin(), emptyCells.end(), rng);

    int numEnemies = std::min(10, (int)emptyCells.size());
    for (int i = 0; i < numEnemies; ++i) {
        int x = emptyCells[i].first;
        int y = emptyCells[i].second;
        float vx = (rng() % 2 - 0.5f) * 2.0f, vz = (rng() % 2 - 0.5f) * 2.0f;
        // enemies.push_back({Vec3{float((x - 7)*1.5f), 1, float((y - 7)*1.5f)}, true, glm::vec3(vx, 0, vz)});
        enemies.push_back({Vec3{float(x), 1, float(y)}, true, glm::vec3(vx, 0, vz)});
    }
    // std::cout << "3" << std::endl;
}

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

GLuint createTextShaderProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, textVertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, textFragmentShaderSrc);
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

// --- Player movement and collision ---
void process_input(GLFWwindow* window) {
    float speed = 5.0f * deltaTime;
    glm::vec3 nextPos = camPos;
    glm::vec3 flatFront = glm::normalize(glm::vec3(camFront.x, 0, camFront.z)); // Ignore Y for movement
    glm::vec3 right = glm::normalize(glm::cross(flatFront, camUp));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) nextPos += flatFront * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) nextPos -= flatFront * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) nextPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) nextPos += right * speed;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) spawnEnemies();
    // Prevent going below ground
    if (nextPos.y < 1.6f) nextPos.y = 1.6f;

    // Maze collision (simple AABB vs wall cubes)
    int px = int(std::round(nextPos.x/1.5f+7)), pz = int(std::round(nextPos.z/1.5f+7));
    if (px>=0 && px<MAZE_W && pz>=0 && pz<MAZE_H && maze[pz][px]==0)
        camPos = nextPos;
    

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJumping) {
        camYVelocity = 6.0f; // jump strength
        isJumping = true;
    }

    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
    static double lastToggle = 0.0;
    double now = glfwGetTime();
    if (now - lastToggle > 0.3) {  // Debounce
        params.showDebug = !params.showDebug;
        lastToggle = now;
    }
}
}
// --- Ray-box intersection for shooting ---
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

// --- Shooting using raytracing ---
void shoot() {
    // PlaySound(TEXT("assets/shoot.wav"), NULL, SND_ASYNC | SND_FILENAME);
    #ifdef _WIN32
    PlaySound(TEXT("assets/shoot.wav"), NULL, SND_ASYNC | SND_FILENAME);
#elif defined(__APPLE__)
    system("afplay assets/shoot.wav &");
#else
    system("aplay assets/shoot.wav &");
#endif

 // Spawn a bullet at camera position, in camera direction
    Bullet b;
    b.pos = camPos + glm::vec3(0, -0.1f, 0); // Slightly below eye
    b.dir = glm::normalize(camFront);
    b.speed = 18.0f;
    b.alive = true;
    bullets.push_back(b);

    float closestT = 1e9f;
    Enemy* hitEnemy = nullptr;
    glm::vec3 rayOrigin = camPos;
    glm::vec3 rayDir = glm::normalize(camFront);

    for (auto& e : enemies) {
        if (!e.alive) continue;
        float tHit;
        // Use world coordinates for the enemy's box center
glm::vec3 enemyWorldPos = glm::vec3(e.pos.x * 1.5f - 10.5f, e.pos.y, e.pos.z * 1.5f - 10.5f);
if (rayIntersectsAABB(rayOrigin, rayDir, enemyWorldPos, 0.175f, tHit)) // 0.175f matches enemy's half-size

       {

           if (tHit > 0.0f && tHit < closestT && tHit < 100.0f) {
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
void drawTextShader(GLuint textShader, const char* text, float x, float y, float scale = 1.0f, glm::vec3 color = glm::vec3(1,1,0)) {
    char buffer[99999];
    int num_quads = stb_easy_font_print(0, 0, (char*)text, NULL, buffer, sizeof(buffer));
    
    std::vector<float> vertices;
    for (int i = 0; i < num_quads; ++i) {
        // Get the 4 vertices of this quad
        float* quad = (float*)(&buffer[i * 16]);
        
        // First triangle of quad (0,1,2)
        vertices.push_back(quad[0]); vertices.push_back(quad[1]); // pos
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
        
        vertices.push_back(quad[2]); vertices.push_back(quad[3]); // pos
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
        
        vertices.push_back(quad[4]); vertices.push_back(quad[5]); // pos
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
        
        // Second triangle of quad (2,3,0)
        vertices.push_back(quad[4]); vertices.push_back(quad[5]); // pos
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
        
        vertices.push_back(quad[6]); vertices.push_back(quad[7]); // pos
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
        
        vertices.push_back(quad[0]); vertices.push_back(quad[1]); // pos
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
    }

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute (2 floats)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glUseProgram(textShader);
    glUniform2f(glGetUniformLocation(textShader, "uOffset"), x, y);
    glUniform1f(glGetUniformLocation(textShader, "uScale"), scale);
    
    // Draw as triangles, 6 vertices per quad
    glDrawArrays(GL_TRIANGLES, 0, num_quads * 6);
    
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

// // glDrawArrays(GL_TRIANGLES, 0, num_quads * 6);
//     GLuint vao, vbo;
//     glGenVertexArrays(1, &vao);
//     glGenBuffers(1, &vbo);
//     glBindVertexArray(vao);
//     glBindBuffer(GL_ARRAY_BUFFER, vbo);
//     glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
//     glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
//     glEnableVertexAttribArray(0);
//     glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
//     glEnableVertexAttribArray(1);

//     glUseProgram(textShader);
//     glUniform2f(glGetUniformLocation(textShader, "uOffset"), x, y);
//     glUniform1f(glGetUniformLocation(textShader, "uScale"), scale);

//     glDrawArrays(GL_QUADS, 0, num_quads * 4);

//     glBindVertexArray(0);
//     glDeleteBuffers(1, &vbo);
//     glDeleteVertexArrays(1, &vao);
// }

// Global state
bool gameOver = false;
bool prevMousePressed = false;
bool anyAlive = false;


int main() {
    if (!glfwInit()) return -1;
    // Request OpenGL 3.3 Core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Simple FPS Maze", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    // glfwSetWindowPos(window, 800, 800);

    // Create ImGui window
    GLFWwindow* imguiWindow = glfwCreateWindow(400, 600, "Debug Controls", NULL, NULL);
    if (!imguiWindow) {
        glfwDestroyWindow(imguiWindow);
        glfwTerminate();
        return -1;
    }

    
    glfwSetWindowPos(window, 100, 100);
    glfwSetWindowPos(imguiWindow, 1320, 100);


    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();

     IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(imguiWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();

    glEnable(GL_DEPTH_TEST);

    // Compile shaders
    GLuint shader = createShaderProgram();

    GLuint textShader = createTextShaderProgram();

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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); // texcoord
    glEnableVertexAttribArray(1);

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

    GLuint floorTexture = loadTexture("assets/floor.jpg");
    GLuint wallTexture = loadTexture("assets/wall.jpg"); 
    GLuint enemyTexture = loadTexture("assets/enemy.jpg"); 

    GLuint skyTexture = loadTexture("assets/sky.jpg");
    
    // // --- Maze and enemy setup ---
    generateMaze();
    for (int y = 0; y < MAZE_H; ++y) {
        for (int x = 0; x < MAZE_W; ++x)
            std::cout << (maze[y][x] ? '#' : '.');
        std::cout << std::endl;
    }
    buildWalls();
    std::cout << "Walls: " << wallPositions.size() << std::endl;
    spawnEnemies();
    std::cout << "Enemies: " << enemies.size() << std::endl;
    camPos = glm::vec3((1-7)*1.5f, 1.6f, (1-7)*1.5f); // Start at maze entrance

    // Crosshair setup (static, only create once)
    float crosshairVertices[] = {
        -0.03f,  0.0f, 0.0f,
         0.03f,  0.0f, 0.0f,
         0.0f, -0.03f, 0.0f,
         0.0f,  0.03f, 0.0f
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
// while (!glfwWindowShouldClose(window)) {
//     glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glfwSwapBuffers(window);
//     glfwPollEvents();
// }

    while (!glfwWindowShouldClose(window) && !glfwWindowShouldClose(imguiWindow)) {
        glfwMakeContextCurrent(window);
        process_input(window);
        if (gameOver) {
            // Clear with dark red background
            glClearColor(0.1f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            // Disable depth test and blending for text
            glDisable(GL_DEPTH_TEST);
            
            // Get window size
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            
            // Set text properties
            const char* msg = "GAME OVER";
            float x = -0.3f;        // Center horizontally (-1 to 1)
            float y = 0.0f;         // Center vertically (-1 to 1)
            float scale = 0.008f;   // Make text larger (was 0.005f)
            
            // Draw text in white color
            drawTextShader(textShader, msg, x, y, scale, glm::vec3(1.0f, 1.0f, 1.0f));
            
            // Re-enable depth test
            glEnable(GL_DEPTH_TEST);
            
            glfwSwapBuffers(window);
            glfwPollEvents();
            
            // Check for restart
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                gameOver = false;
                camPos = glm::vec3((1-7)*1.5f, 1.6f, (1-7)*1.5f);
                camYVelocity = 0.0f;
                isJumping = false;
                spawnEnemies();
            }
            
            continue;
        }
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        

        // Update bullets
        for (auto& b : bullets) {
            if (!b.alive) continue;
            b.pos += b.dir * b.speed * deltaTime;
            // Remove bullet if too far
            if (glm::length(b.pos - camPos) > 50.0f) b.alive = false;

            // Check collision with enemies
            for (auto& e : enemies) {
                if (!e.alive || e.smashing) continue;
                glm::vec3 enemyWorld = glm::vec3(e.pos.x * 1.5f - 10.5f, e.pos.y, e.pos.z * 1.5f - 10.5f);
                float dist = glm::distance(glm::vec3(b.pos.x, 1.0f, b.pos.z), glm::vec3(enemyWorld.x, 1.0f, enemyWorld.z));
                if (dist < 0.35f) { // Adjust threshold as needed
                    e.smashing = true;
                    e.smashTime = 0.0f;
                    b.alive = false;
                }
            }
        }

        // Gravity and jump
        const float gravity = -15.0f;
        float groundY = 1.6f;
        if (isJumping) {
            camYVelocity += gravity * deltaTime;
            camPos.y += camYVelocity * deltaTime;
            if (camPos.y <= groundY) {
                camPos.y = groundY;
                camYVelocity = 0.0f;
                isJumping = false;
            }
        }
        // Prevent player from going below ground even if not jumping
        if (camPos.y < groundY) camPos.y = groundY;

        // Mouse shooting (one shot per click)
        bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (mousePressed && !prevMousePressed) {
            shoot();
        }
        prevMousePressed = mousePressed;
        
        for (auto& e : enemies) {
            if (!e.alive) continue;
            if (e.smashing) {
                e.smashTime += deltaTime;
                if (e.smashTime > 0.5f) { // Animation lasts 0.5s
                    e.alive = false;
                    e.smashing = false;
                }
                continue; // Don't move while smashing
            }
            anyAlive = true;
            // Move in grid coordinates
            glm::vec3 next = glm::vec3(e.pos.x, e.pos.y, e.pos.z) + e.velocity * deltaTime;
            int ex = int(std::round(next.x)), ez = int(std::round(next.z));
            if (ex >= 0 && ex < MAZE_W && ez >= 0 && ez < MAZE_H && maze[ez][ex] == 0) {
                e.pos.x = next.x;
                e.pos.z = next.z;
            } else {
                // Bounce and randomize direction a bit
                e.velocity.x = -e.velocity.x + ((rand()%100)/100.0f-0.5f)*0.25f;
                e.velocity.z = -e.velocity.z + ((rand()%100)/100.0f-0.5f)*0.25f;
            }

            // Check collision with player
            glm::vec3 enemyWorld = glm::vec3(e.pos.x * 1.5f - 10.5f, e.pos.y, e.pos.z * 1.5f - 10.5f);
            float dist = glm::distance(glm::vec3(camPos.x, 1.0f, camPos.z), glm::vec3(enemyWorld.x, 1.0f, enemyWorld.z));
            if (dist < 0.4f) { // Adjust threshold as needed
                gameOver = true;
            }
        }
        anyAlive = false;
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

        glEnable(GL_DEPTH_TEST);
        // if (!anyAlive) {
        //     const char* msg = "YOU WON!";
        //     int textPixelWidth = stb_easy_font_width((char*)msg);
        //     int textPixelHeight = stb_easy_font_height((char*)msg);

        //     // Calculate scale so text height is about 1/8 of window height
        //     float scale = 2.0f / height * 32.0f; // 32 is a good font size for 800px window

        //     // Center in NDC
        //     float x = -((float)textPixelWidth * scale) / 2.0f;
        //     float y = -((float)textPixelHeight * scale) / 2.0f;

        //     drawTextShader(textShader, msg, x, y, scale, glm::vec3(1,1,0));
        //     glfwSwapBuffers(window);
        //     glfwPollEvents();
        //     continue;
        // }
        // Draw maze walls
        for (auto& pos : wallPositions) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(1.5f, 2.0f, 1.5f)); // 2.0f = taller, 1.5f = wider path
            glm::mat4 mvp = projection * view * model;
            drawObject(cubeVAO, shader, 36, mvp, glm::vec3(0.5f,0.5f,0.5f), wallTexture);
        }

        // Draw enemies
        for (auto& e : enemies) {
            if (!e.alive) continue;
            glm::vec3 color = glm::vec3(1,0,0);
            float smashScaleY = 0.35f;
            float smashAlpha = 1.0f;
            if (e.smashing) {
                float t = e.smashTime / 0.5f;
                smashScaleY = 0.35f * (1.0f - t); // Shrink Y
                smashAlpha = 1.0f - t;            // Fade out
            }
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(e.pos.x * 1.5f - 10.5f, e.pos.y, e.pos.z * 1.5f - 10.5f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(0.35f, smashScaleY, 0.35f));
            glm::mat4 mvp = projection * view * model;
            // If you want to pass alpha, modify your shader to accept it, or just use color for now
            drawObject(cubeVAO, shader, 36, mvp, color, enemyTexture);
        }

        for (auto& b : bullets) {
            if (!b.alive) continue;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), b.pos) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(0.08f, 0.08f, 0.08f));
            glm::mat4 mvp = projection * view * model;
            drawObject(cubeVAO, shader, 36, mvp, glm::vec3(1,1,0));
        }

        // Draw a hand with gun (bigger gun quad, offset to lower right)
        glDisable(GL_DEPTH_TEST);
        glm::mat4 handModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.3f, -0.3f, 0.0f)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(2.8f, 2.0f, 1.0f));
        glm::mat4 handMVP = handModel; // NDC
        drawObject(gunVAO, shader, 6, handMVP, glm::vec3(0.4f, 0.3f, 0.2f));
        glEnable(GL_DEPTH_TEST);

        // Draw a crosshair in the center of the screen
        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader, "uMVP"), 1, GL_FALSE, &glm::mat4(1.0f)[0][0]);
        glUniform3fv(glGetUniformLocation(shader, "uColor"), 1, &glm::vec3(1,1,1)[0]);
        glUniform1i(glGetUniformLocation(shader, "useTex"), 0);
        glBindVertexArray(crossVAO);
        glDrawArrays(GL_LINES, 0, 2);
        glDrawArrays(GL_LINES, 2, 2);
        glBindVertexArray(0);

                // Disable depth writing so sky is always in the background
        glDepthMask(GL_FALSE);

        // Center skybox at camera, scale large (e.g., 50 units)
        glm::mat4 skyModel = glm::translate(glm::mat4(1.0f), camPos) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(50.0f));

        // Remove translation from view matrix so skybox doesn't move with camera position
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        glm::mat4 skyMVP = projection * skyView * skyModel;
        glCullFace(GL_FRONT);
        // Draw with sky texture (use your cube VAO)
        drawObject(cubeVAO, shader, 36, skyMVP, glm::vec3(1.0f), skyTexture);
        glCullFace(GL_BACK);
        // Re-enable depth writing
        glDepthMask(GL_TRUE);



            if (params.showDebug) {
                    
                glfwMakeContextCurrent(imguiWindow);
                // Before glfwSwapBuffers(window)...
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                // Make ImGui window fill the entire window
                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(ImVec2(400, 600));
                ImGui::Begin("Game Parameters", nullptr, 
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        
                    // ImGui::Begin("Game Parameters");
                    ImGui::SliderFloat("Player Speed", &params.playerSpeed, 1.0f, 20.0f);
                    ImGui::SliderFloat("Jump Strength", &params.jumpStrength, 1.0f, 15.0f);
                    ImGui::SliderFloat("Enemy Speed", &params.enemySpeed, 0.5f, 10.0f);
                    ImGui::SliderFloat("Bullet Speed", &params.bulletSpeed, 5.0f, 50.0f);
                    ImGui::ColorEdit3("Enemy Color", &params.enemyColor.x);
                    ImGui::SliderFloat("Wall Height", &params.wallHeight, 1.0f, 5.0f);
                    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                    ImGui::End();
            }

            ImGui::Render();
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwSwapBuffers(imguiWindow);
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
    glDeleteVertexArrays(1, &crossVAO);
    glDeleteBuffers(1, &crossVBO);
    glDeleteProgram(shader);

    ImGui_ImplOpenGL3_Shutdown();
ImGui_ImplGlfw_Shutdown();
ImGui::DestroyContext();


glfwDestroyWindow(imguiWindow);
    glfwDestroyWindow(window);

    glfwTerminate();
    return 0;
}