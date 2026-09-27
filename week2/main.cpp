#include <glad/gl.h>      // GLAD заголовогы GLFW-дан бұрын орналасуы керек
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH  = 1280;
const int HEIGHT = 720;
bool isSpacePressed = false;

// ---------------------------------------------------------------------
// GLSL Шейдерлері
// ---------------------------------------------------------------------
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;
    layout (location = 2) in float aIsDynamic; // 1.0 = динамикалық (бұрынғы қара), 0.0 = сарғыш

    out vec3 ourColor;
    out float isDynamic;

    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        ourColor = aColor;
        isDynamic = aIsDynamic;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 ourColor;
    in float isDynamic;

    out vec4 FragColor;

    uniform vec3 uBgColor; // Динамикалық фон/қара үшбұрыштар түсі

    void main() {
        if (isDynamic > 0.5) {
            // Динамикалық үшбұрыштар фон түсін қабылдайды
            FragColor = vec4(uBgColor, 1.0f);
        } else {
            // Статикалық сарғыш үшбұрыштар
            FragColor = vec4(ourColor, 1.0f);
        }
    }
)";

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        isSpacePressed = true;
    } else {
        isSpacePressed = false;
    } 
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Компьютерлік графика — 2-апта", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(0); // VSync өшіру

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    // -----------------------------------------------------------------
    // Шейдерлерді компиляциялау
    // -----------------------------------------------------------------
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    // -----------------------------------------------------------------
    // 6 ЖОЛ x 4 БАҒАН ТОРЫН ГЕНЕРАЦИЯЛАУ
    // -----------------------------------------------------------------
    std::vector<float> vertices;
    int rows = 6;
    int cols = 4;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float x0 = -0.8f + c * 0.4f;
            float x1 = x0 + 0.35f;
            float y0 = -0.8f + r * 0.26f;
            float y1 = y0 + 0.22f;

            // Сарғыш түс
            float oR = 0.9f, oG = 0.5f, oB = 0.2f;

            // 1. Сарғыш үшбұрыш (статикалық, isDynamic = 0.0)
            vertices.insert(vertices.end(), {
                x0, y0, oR, oG, oB, 0.0f,
                x1, y0, oR, oG, oB, 0.0f,
                x0, y1, oR, oG, oB, 0.0f
            });

            // 2. Фонмен бірге өзгеретін үшбұрыш (динамикалық, isDynamic = 1.0)
            vertices.insert(vertices.end(), {
                x1, y0, 0.0f, 0.0f, 0.0f, 1.0f,
                x1, y1, 0.0f, 0.0f, 0.0f, 1.0f,
                x0, y1, 0.0f, 0.0f, 0.0f, 1.0f
            });
        }
    }

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Координаталар (x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Түстер (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Динамикалық белгі (isDynamic)
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    int uBgColorLoc = glGetUniformLocation(shaderProgram, "uBgColor");

    double lastTime = glfwGetTime();
    int frameCount = 0;

    // -----------------------------------------------------------------
    // Негізгі цикл
    // -----------------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - lastTime >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }

        processInput(window);

        // Динамикалық фон және қара үшбұрыштар түсін есептеу
        float bgR, bgG, bgB;

        if (isSpacePressed) {
            // Пробел басылғанда ақ түс
            bgR = 1.0f; bgG = 1.0f; bgB = 1.0f;
        } else {
            // Басқа уақытта динамикалық өзгеретін түс
            float t = (float)glfwGetTime();
            bgR = (std::sin(t * 4.0f) + 1.0f) * 0.5f * 0.3f;
            bgG = (std::sin(t * 3.0f) + 1.0f) * 0.5f * 0.3f;
            bgB = 0.35f;
        }

        // Фонды тазалау
        glClearColor(bgR, bgG, bgB, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Шейдерді іске қосу және түсті жіберу
        glUseProgram(shaderProgram);
        glUniform3f(uBgColorLoc, bgR, bgG, bgB);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}