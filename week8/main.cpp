// 8-апта: тереңдік буфері, back-face culling, Z-fighting, бірнеше объект
//
// Пернелер:
//   T   : тереңдік тесті қосу/өшіру
//   Z   : Z-fighting көрсету (екі куб бетпе-бет) қосу/өшіру
//   B   : far-ды 100 <-> 1000 ауыстыру
//   C   : culling қосу/өшіру
//   F   : culling GL_BACK <-> GL_FRONT
//   Tab : wireframe қосу/өшіру
//   ESC : шығу

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

// Терезе өлшемі (framebuffer_size_callback ішінде жаңартылады)
int gWidth = 800, gHeight = 600;

// Пернемен өзгертілетін күйлер
bool gDepth = true;        // тереңдік тесті
bool gZFight = false;      // Z-fighting режимі
bool gCull = true;         // culling қосулы ма
bool gCullFront = false;   // false: GL_BACK, true: GL_FRONT
bool gWire = false;        // wireframe
float gNear = 0.1f;
float gFar = 100.0f;

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
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);   // тереңдік буферін сұраймыз

    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "Week 8", nullptr, nullptr);
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

    // 36 вершина. МАҢЫЗДЫ: culling үшін барлық қыр сырттан қарағанда
    // сағат тіліне ҚАРСЫ (CCW) жазылуы керек. 7-аптадағы массивте қырлар әртүрлі
    // бағытта еді, сондықтан culling қосқанда дұрыс қырлар жоғалар еді.
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

    // Тереңдік буферінің биттерін тексеру
    int bits = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH,
                                          GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &bits);
    std::cout << "depth bits: " << bits << std::endl;
    std::cout << "T depth | Z z-fight | B far | C cull | F cull front/back | Tab wireframe" << std::endl;

    glFrontFace(GL_CCW);   // сыртқы қыр: сағат тіліне қарсы

    // 5 кубтың орны
    glm::vec3 positions[] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f, 0.5f, -2.0f),
        glm::vec3(-1.8f, -0.3f, -1.0f),
        glm::vec3(1.2f, -0.8f, -3.5f),
        glm::vec3(-2.2f, 1.0f, -4.0f)};

    while (!glfwWindowShouldClose(window)) {
        // Пернемен ауыстырылатын күйлерді әр кадрда қолданамыз
        if (gDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (gCull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        glCullFace(gCullFront ? GL_FRONT : GL_BACK);
        glPolygonMode(GL_FRONT_AND_BACK, gWire ? GL_LINE : GL_FILL);

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   // тереңдік буферін де тазалаймыз

        float angle = (float)glfwGetTime();
        float aspect = (gHeight > 0) ? (float)gWidth / (float)gHeight : 1.0f;

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, gNear, gFar);
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                     glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));

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