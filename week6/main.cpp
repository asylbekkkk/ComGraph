#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

const int WIDTH  = 1280;
const int HEIGHT = 720;

bool wireframe = false;
bool prevKeys[GLFW_KEY_LAST + 1] = { false };

bool pressedOnce(GLFWwindow* window, int key) {
    bool now = glfwGetKey(window, key) == GLFW_PRESS;
    bool fired = now && !prevKeys[key];
    prevKeys[key] = now;
    return fired;
}

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (pressedOnce(window, GLFW_KEY_TAB)) {
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    }
}

unsigned int makeShader(const char* vsSrc, const char* fsSrc) {
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
    int ok;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(vs, 1024, nullptr, log); std::cerr << log << "\n"; }

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(fs, 1024, nullptr, log); std::cerr << log << "\n"; }

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[1024]; glGetProgramInfoLog(prog, 1024, nullptr, log); std::cerr << log << "\n"; }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform vec2 uOffset;

out vec3 vColor;

void main() {
    gl_Position = vec4(aPos.xy + uOffset, aPos.z, 1.0);
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
)";

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW iske qosylmady\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Shar zhane Tortburysh - No Collision", nullptr, nullptr);
    if (!window) {
        std::cerr << "Tereze zhasalmady.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(1);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD zhuktelmedi\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";
    std::cout << "BASQARU: TAB - wireframe, ESC - shygu\n";

    const int STRIDE = 6 * sizeof(float);
    float aspect = (float)WIDTH / (float)HEIGHT;

    // =================================================================
    // 1. ШАР (Идеалды домалақ)
    // =================================================================
    const int circleSegments = 30;
    float circleVertices[(circleSegments + 1) * 6];
    unsigned int circleIndices[circleSegments * 3];

    circleVertices[0] = 0.0f;
    circleVertices[1] = 0.0f;
    circleVertices[2] = 0.0f;
    circleVertices[3] = 1.0f;
    circleVertices[4] = 0.3f;
    circleVertices[5] = 0.1f;

    const float PI = 3.14159265f;
    float radius = 0.11f;

    for (int i = 0; i < circleSegments; i++) {
        float theta = 2.0f * PI * float(i) / float(circleSegments);
        int idx = (i + 1) * 6;
        circleVertices[idx + 0] = (radius * std::cos(theta)) / aspect;
        circleVertices[idx + 1] = radius * std::sin(theta);
        circleVertices[idx + 2] = 0.0f;
        
        circleVertices[idx + 3] = 0.9f;
        circleVertices[idx + 4] = 0.5f;
        circleVertices[idx + 5] = 1.0f;
    }

    for (int i = 0; i < circleSegments; i++) {
        circleIndices[i * 3 + 0] = 0;
        circleIndices[i * 3 + 1] = i + 1;
        circleIndices[i * 3 + 2] = (i + 1) % circleSegments + 1;
    }

    unsigned int vaoCircle, vboCircle, eboCircle;
    glGenVertexArrays(1, &vaoCircle);
    glGenBuffers(1, &vboCircle);
    glGenBuffers(1, &eboCircle);

    glBindVertexArray(vaoCircle);
    glBindBuffer(GL_ARRAY_BUFFER, vboCircle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(circleVertices), circleVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboCircle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(circleIndices), circleIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // =================================================================
    // 2. ТӨРТБҰРЫШ (Идеалды шаршы)
    // =================================================================
    float halfSize = 0.12f;
    float quadVertices[] = {
         halfSize / aspect,  halfSize, 0.0f,   0.2f, 0.8f, 1.0f,
         halfSize / aspect, -halfSize, 0.0f,   0.1f, 0.5f, 0.8f,
        -halfSize / aspect, -halfSize, 0.0f,   0.1f, 0.5f, 0.8f,
        -halfSize / aspect,  halfSize, 0.0f,   0.2f, 0.8f, 1.0f
    };

    unsigned int quadIndices[] = {
        0, 1, 3,
        1, 2, 3
    };

    unsigned int vaoQuad, vboQuad, eboQuad;
    glGenVertexArrays(1, &vaoQuad);
    glGenBuffers(1, &vboQuad);
    glGenBuffers(1, &eboQuad);

    glBindVertexArray(vaoQuad);
    glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboQuad);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // -----------------------------------------------------------------
    unsigned int shader = makeShader(vertexSrc, fragmentSrc);
    int locOffset = glGetUniformLocation(shader, "uOffset");

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        float time = (float)glfwGetTime();

        // --- ЖЕРДЕГІ ҚОЗҒАЛЫС ЖӘНЕ ТАЙМИНГ ---
        float groundY = -0.5f;

        // Төртбұрыш жиілігі, ал шар жиілігі дәл 3 есе көп болуы керек
        const float quadSpeed = 1.1f;
        const float ballSpeed = 3.0f * quadSpeed;   // = 3.3

        // 1. Шар: орта нүктеде тұрып, жоғары-төмен секіреді
        float bounceHeight = 0.65f;
        float ballY = groundY + radius + bounceHeight * std::abs(std::sin(time * ballSpeed));
        float ballX = 0.0f;

        // 2. Төртбұрыш: x = 0 нүктесінен өткен сайын (cos = 0) шар апексте болады
        float quadX = 1.4f * std::cos(time * quadSpeed);
        float quadY = groundY + halfSize;

        // --- СЫЗУ ---
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);

        // Төртбұрышты сызу
        glBindVertexArray(vaoQuad);
        glUniform2f(locOffset, quadX, quadY);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Шарды сызу
        glBindVertexArray(vaoCircle);
        glUniform2f(locOffset, ballX, ballY);
        glDrawElements(GL_TRIANGLES, circleSegments * 3, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Тазалау ---
    glDeleteVertexArrays(1, &vaoCircle);
    glDeleteVertexArrays(1, &vaoQuad);
    glDeleteBuffers(1, &vboCircle);
    glDeleteBuffers(1, &vboQuad);
    glDeleteBuffers(1, &eboCircle);
    glDeleteBuffers(1, &eboQuad);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}