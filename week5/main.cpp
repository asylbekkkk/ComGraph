#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <iostream>

// ---------- Шейдерлер ----------
const char* vertexSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
out vec3 vColor;

uniform vec2  uOffset;   // орбитадағы бағыт: (cos, sin)
uniform float uRadius;   // орбита радиусы
uniform float uScale;    // пульсация

void main() {
    gl_Position = vec4(aPos.xy * uScale + uOffset * uRadius, aPos.z, 1.0);
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

// ---------- Бастапқы мәндер ----------
const float START_SPEED  = 1.5f;   // радиан/секунд
const float START_RADIUS = 0.4f;
const float MIN_SPEED = 0.1f, MAX_SPEED = 10.0f;
const float MIN_RADIUS = 0.1f, MAX_RADIUS = 0.7f;  // 0.25 (үшбұрыш) + радиус ≤ 1.0

// ---------- Жаһандық күй ----------
float speed  = START_SPEED;
float radius = START_RADIUS;
float angle  = 0.0f;

void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Жеделдетуге де dt керек: әйтпесе жылдамдықтың өзгеруі FPS-ке тәуелді болады
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) speed += 2.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) speed -= 2.0f * dt;
    speed = std::clamp(speed, MIN_SPEED, MAX_SPEED);

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) radius += 0.3f * dt;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) radius -= 0.3f * dt;
    radius = std::clamp(radius, MIN_RADIUS, MAX_RADIUS);

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        speed  = START_SPEED;
        radius = START_RADIUS;
        angle  = 0.0f;
    }
}

unsigned int compileShader(GLenum type, const char* src) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
    }
    return shader;
}

unsigned int createProgram(const char* vsSrc, const char* fsSrc) {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Program link error:\n" << log << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Week 04: uniform + delta time", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);   // VSync. 1-тапсырмада 0 қойып тексеріледі

    // x, y, z,   r, g, b
    float vertices[] = {
         0.00f,  0.25f, 0.0f,   1.0f, 0.3f, 0.2f,
        -0.22f, -0.15f, 0.0f,   0.2f, 1.0f, 0.3f,
         0.22f, -0.15f, 0.0f,   0.2f, 0.4f, 1.0f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int shader = createProgram(vertexSrc, fragmentSrc);

    // Uniform орындарын циклге дейін БІР рет табамыз
    int locOffset = glGetUniformLocation(shader, "uOffset");
    int locScale  = glGetUniformLocation(shader, "uScale");
    int locRadius = glGetUniformLocation(shader, "uRadius");

    float lastFrame = (float)glfwGetTime();   // НАЗАР: нөл емес
    double fpsTimer = glfwGetTime();
    int frames = 0;

    while (!glfwWindowShouldClose(window)) {
        // --- Уақыт ---
        float now = (float)glfwGetTime();
        float dt  = now - lastFrame;
        lastFrame = now;

        processInput(window, dt);
        angle += speed * dt;

        // Пульсация: sin ∈ [-1, 1] → (sin+1)/2 ∈ [0, 1] → 0.5 + 0.5*... ∈ [0.5, 1.0]
        float scale = 0.5f + 0.5f * (std::sin(now * 3.0f) + 1.0f) * 0.5f;

        glClearColor(0.08f, 0.09f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);                     // МІНДЕТТІ, әрі БІРІНШІ
        glUniform1f(locScale, scale);
        glUniform1f(locRadius, radius);
        glBindVertexArray(VAO);

        // 1-объект: сағат тіліне қарсы
        glUniform2f(locOffset, std::cos(angle), std::sin(angle));
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 2-объект: сағат тілімен (бұрыш теріс), кішірек орбитада
        glUniform1f(locRadius, radius * 0.5f);
        glUniform2f(locOffset, std::cos(-angle), std::sin(-angle));
        glDrawArrays(GL_TRIANGLES, 0, 3);

        frames++;
        if (glfwGetTime() - fpsTimer >= 1.0) {
            std::cout << "FPS: " << frames
                      << "  speed: " << speed << " rad/s"
                      << "  radius: " << radius << std::endl;
            frames = 0;
            fpsTimer = glfwGetTime();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader);
    glfwTerminate();
}