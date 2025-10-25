#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include "stb_easy_font.h"
#include <glm/glm.hpp>

// Vertex/fragment shaders for text
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

void drawTextShader(GLuint textShader, const char* text, float x, float y, float scale = 1.0f, glm::vec3 color = glm::vec3(1,1,0)) {
    char buffer[99999];
    int num_quads = stb_easy_font_print(0, 0, (char*)text, NULL, buffer, sizeof(buffer));
    std::vector<float> vertices;
    for (int i = 0; i < num_quads * 4; ++i) {
        float* v = (float*)(&buffer[i * 16]);
        vertices.push_back(v[0]);
        vertices.push_back(v[1]);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
    }
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glUseProgram(textShader);
    glUniform2f(glGetUniformLocation(textShader, "uOffset"), x, y);
    glUniform1f(glGetUniformLocation(textShader, "uScale"), scale);

    glDrawArrays(GL_QUADS, 0, num_quads * 4);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

int main() {
    if (!glfwInit()) return -1;

    // Request OpenGL 3.3 Core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "drawTextShader Test", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    GLuint textShader = createTextShaderProgram();

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Center "Hello, World!" in NDC
        const char* msg = "Hello, World!";
        char buffer[99999];
        float textPixelWidth = 0, textPixelHeight = 0;
        int num_quads = stb_easy_font_print(0, 0, (char*)msg, NULL, buffer, sizeof(buffer));
        if (num_quads > 0) {
            for (int i = 0; i < num_quads * 4; ++i) {
                float* v = (float*)(&buffer[i * 16]);
                if (v[0] > textPixelWidth) textPixelWidth = v[0];
                if (v[1] > textPixelHeight) textPixelHeight = v[1];
            }
        }
        float scale = 2.0f / height * 32.0f;
        float x = -((float)textPixelWidth * scale) / 2.0f;
        float y = -((float)textPixelHeight * scale) / 2.0f;

        drawTextShader(textShader, msg, x, y, scale, glm::vec3(1,1,0));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}