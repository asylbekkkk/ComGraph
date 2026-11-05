// 10-апта: толық еркін камера. Тінтуірмен бағыт (yaw/pitch), дөңгелекпен fov.
// Негізі: 9-аптадағы камера (camPos, camFront, camUp, WASD, dt).
//
// Камера:
//   Тінтуір      : айналу (yaw/pitch)
//   Дөңгелек     : zoom (fov 1..90)
//   W / S        : алға / артқа
//   A / D        : солға / оңға
//   Space        : жоғары
//   Left Shift   : төмен
//   Left Ctrl    : жүгіру (жылдамдық 2 есе)
//   I            : Invert Y қосу/өшіру
//   P            : камера күйін консольге шығару
//   R            : камераны бастапқы күйге қайтару
// 8-аптадағы пернелер:
//   T depth | Z z-fight | B far | C cull | F cull front/back | Tab wireframe | ESC шығу

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <cstdio>

// Терезе өлшемі (framebuffer_size_callback ішінде жаңартылады)
int gWidth = 800, gHeight = 600;

// Пернемен өзгертілетін күйлер (8-апта)
bool gDepth = true;        // тереңдік тесті
bool gZFight = false;      // Z-fighting режимі
bool gCull = true;         // culling қосулы ма
bool gCullFront = false;   // false: GL_BACK, true: GL_FRONT
bool gWire = false;        // wireframe
float gNear = 0.1f;
float gFar = 100.0f;

// Камера айнымалылары (жаһандық, циклдің сыртында)
glm::vec3 camPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camUp = glm::vec3(0.0f, 1.0f, 0.0f);

// 10-апта: тінтуір және zoom айнымалылары
float yaw = -90.0f;        // -90: камера -Z жаққа (кубтарға) қарайды
float pitch = 0.0f;
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;
float fov = 45.0f;
bool invertY = false;      // 3-тапсырма: Invert Y

// ---------- Шейдерлер ----------
const char* vertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

// uTint: Z-fighting кезінде екінші кубты қараңғылау ету үшін (әдетте 1,1,1)
const char* fragmentSrc = R"(#version 330 core
in vec3 vColor;
out vec4 FragColor;
uniform vec3 uTint;
void main() {
    FragColor = vec4(vColor * uTint, 1.0);
}
)";

unsigned int compileShader(GLenum type, const char* src) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Шейдер қатесі: " << log << std::endl;
    }
    return shader;
}

unsigned int createProgram() {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Линковка қатесі: " << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    gWidth = width;
    gHeight = height;
    glViewport(0, 0, width, height);
}

// 3-қадам: тінтуір қозғалғанда yaw мен pitch жаңарады, camFront қайта есептеледі
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    std::cout << "mouse: " << xpos << ", " << ypos << std::endl;
    if (firstMouse) {   // бірінші шақыруда камера секіріп кетпеуі үшін
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;  
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;   // экранның y-і төмен қарай өседі, сондықтан кері
    if (invertY) yoffset = -yoffset;       // 3-тапсырма
    lastX = (float)xpos;
    lastY = (float)ypos;

    const float sensitivity = 0.1f;        // 2-тапсырма: 0.03, 0.1, 0.5 мәндерін сынап көр
    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    // 1-тапсырма: тәжірибе үшін келесі екі жолды комментке ал, сосын қайтар
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    // std::cout << "pitch = " << pitch << std::endl;   // 1-тапсырма: pitch-ті бақылау

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camFront = glm::normalize(dir);
}

// 4-қадам: дөңгелек fov-ты өзгертеді
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 90.0f) fov = 90.0f;   // 5-тапсырма: жоғарғы шек 45-тен 90-ға көтерілді
}

// 4-тапсырма: камераның толық күйін шығару
void printCameraState() {
    printf("camPos = (%.2f, %.2f, %.2f)\n", camPos.x, camPos.y, camPos.z);
    printf("camFront = (%.2f, %.2f, %.2f)\n", camFront.x, camFront.y, camFront.z);
    printf("yaw = %.1f pitch = %.1f fov = %.1f\n", yaw, pitch, fov);
    printf("length = %.6f\n", glm::length(camFront));
}

// Бір рет басылатын пернелер (ауыстырғыштар)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        case GLFW_KEY_T:
            gDepth = !gDepth;
            std::cout << "depth test: " << gDepth << std::endl;
            break;
        case GLFW_KEY_C:
            gCull = !gCull;
            std::cout << "cull: " << gCull << std::endl;
            break;
        case GLFW_KEY_F:
            gCullFront = !gCullFront;
            std::cout << (gCullFront ? "cull FRONT" : "cull BACK") << std::endl;
            break;
        case GLFW_KEY_Z:
            gZFight = !gZFight;
            gNear = gZFight ? 0.0001f : 0.1f;
            gFar = gZFight ? 1000.0f : 100.0f;
            std::cout << "z-fight: " << gZFight << " near=" << gNear << " far=" << gFar << std::endl;
            break;
        case GLFW_KEY_B:
            gFar = (gFar == 1000.0f) ? 100.0f : 1000.0f;
            std::cout << "far=" << gFar << std::endl;
            break;
        case GLFW_KEY_TAB:
            gWire = !gWire;
            break;
        case GLFW_KEY_R:   // қосымша: камераны бастапқы күйге қайтару
            camPos = glm::vec3(0.0f, 0.0f, 3.0f);
            camFront = glm::vec3(0.0f, 0.0f, -1.0f);
            yaw = -90.0f;
            pitch = 0.0f;
            fov = 45.0f;
            break;
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);   // тереңдік буферін сұраймыз

    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "Week 10", nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе құрылмады" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "GLAD іске қосылмады" << std::endl;
        return -1;
    }

    // 2-қадам: курсорды терезеге ұстап, тінтуір мен дөңгелекті тіркеу
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 36 вершина, барлық қыр сырттан қарағанда сағат тіліне қарсы (CCW)
    float vertices[] = {
        // артқы қыр (-z), қызыл
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        // алдыңғы қыр (+z), жасыл
        -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
        // сол қыр (-x), көк
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        // оң қыр (+x), сары
         0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
        // төменгі қыр (-y), көгілдір
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
        // жоғарғы қыр (+y), қызғылт
        -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 1.0f};

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int shader = createProgram();

    // uniform орындарын циклге дейін бір рет сұраймыз
    int locModel = glGetUniformLocation(shader, "uModel");
    int locView = glGetUniformLocation(shader, "uView");
    int locProj = glGetUniformLocation(shader, "uProjection");
    int locTint = glGetUniformLocation(shader, "uTint");

    glFrontFace(GL_CCW);   // сыртқы қыр: сағат тіліне қарсы

    // 5 кубтың орны (сахна өзгермейді)
    glm::vec3 positions[] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f, 0.5f, -2.0f),
        glm::vec3(-1.8f, -0.3f, -1.0f),
        glm::vec3(1.2f, -0.8f, -3.5f),
        glm::vec3(-2.2f, 1.0f, -4.0f)};

    float lastFrame = (float)glfwGetTime();   // delta time үшін
    bool iPressedLast = false;                // I пернесінің алдыңғы кадрдағы күйі
    bool pPressedLast = false;                // P пернесінің алдыңғы кадрдағы күйі

    while (!glfwWindowShouldClose(window)) {
        // delta time: өткен кадрдан бергі уақыт
        float currentFrame = (float)glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Пернетақтамен камераны басқару (dt есептелгеннен кейін)
        float speed = 2.5f * dt;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 2.0f;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * camFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * camFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camPos -= glm::normalize(glm::cross(camFront, camUp)) * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camPos += glm::normalize(glm::cross(camFront, camUp)) * speed;

        // Space жоғары, Left Shift төмен (әлемнің Y осі бойымен)
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camPos += speed * glm::vec3(0.0f, 1.0f, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camPos -= speed * glm::vec3(0.0f, 1.0f, 0.0f);

        // 3-тапсырма: I пернесі (Invert Y). Басулы тұрғанда әр кадр ауысып кетпеуі үшін алдыңғы күйді еске сақтаймыз
        bool iNow = glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS;
        if (iNow && !iPressedLast) {
            invertY = !invertY;
            std::cout << "invert Y: " << invertY << std::endl;
        }
        iPressedLast = iNow;

        // 4-тапсырма: P пернесі (камера күйін шығару), сол әдіспен
        bool pNow = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (pNow && !pPressedLast) printCameraState();
        pPressedLast = pNow;

        // Пернемен ауыстырылатын күйлерді әр кадрда қолданамыз (8-апта)
        if (gDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (gCull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        glCullFace(gCullFront ? GL_FRONT : GL_BACK);
        glPolygonMode(GL_FRONT_AND_BACK, gWire ? GL_LINE : GL_FILL);

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = (float)glfwGetTime();
        float aspect = (gHeight > 0) ? (float)gWidth / (float)gHeight : 1.0f;

        // 5-қадам: projection fov айнымалысына байланған (тұрақты 45 емес)
        glm::mat4 projection = glm::perspective(glm::radians(fov), aspect, gNear, gFar);

        // Мақсат камерамен бірге жылжиды: camPos + camFront
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

        glUseProgram(shader);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glBindVertexArray(vao);

        if (!gZFight) {
            // Қалыпты режим: 5 куб, әрқайсысы өз жылдамдығымен айналады
            glUniform3f(locTint, 1.0f, 1.0f, 1.0f);
            for (int i = 0; i < 5; i++) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), positions[i]);
                model = glm::rotate(model, angle * (0.5f + i * 0.3f), glm::vec3(0.3f, 1.0f, 0.0f));
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        } else {
            // Z-fighting: екі куб бетпе-бет, айырмасы тек 0.001
            glm::mat4 first = glm::rotate(glm::mat4(1.0f), 0.4f * sinf(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            glUniform3f(locTint, 1.0f, 1.0f, 1.0f);
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(first));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glm::mat4 second = glm::translate(first, glm::vec3(0.0f, 0.0f, 0.001f));
            glUniform3f(locTint, 0.4f, 0.4f, 0.4f);   // қараңғырақ, жыпылықтау көрінсін
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(second));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);
    glfwTerminate();
    return 0;
}
