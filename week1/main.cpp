#include <glad/glad.h>   // Сіздің жүйеңіздегі GLAD заголовогы
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

// ---------------------------------------------------------------------
//  1-АПТА ТАПСЫРМАЛАРЫНЫҢ БАПТАУЛАРЫ
// ---------------------------------------------------------------------
const int WIDTH  = 1280; // 1. Өлшемі 1280x720
const int HEIGHT = 720;
bool isSpacePressed = false;

// Шейдерлер коды
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;
    out vec3 ourColor;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        ourColor = aColor;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 ourColor;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(ourColor, 1.0f);
    }
)";

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// 3. Пернетақта арқылы Пробел басылғанды тексеру
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
    // -----------------------------------------------------------------
    //  1. GLFW-ны іске қосу
    // -----------------------------------------------------------------
    if (!glfwInit()) {
        std::cerr << "GLFW іске қосылмады\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    // -----------------------------------------------------------------
    //  2. Терезе жасау
    // -----------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
                                          "Компьютерлік графика",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(0); // 4. VSync өшірілген

    // -----------------------------------------------------------------
    //  3. GLAD жүктеу
    // -----------------------------------------------------------------
    if (!gladLoadGL()) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    // -----------------------------------------------------------------
    //  Шейдерлерді құру
    // -----------------------------------------------------------------
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // -----------------------------------------------------------------
    //  СУРЕТТЕГІДЕЙ ТОР (GRID) ҮШБҰРЫШТАРЫН ИНИЦИАЛИЗАЦИЯЛАУ
    // -----------------------------------------------------------------
    std::vector<float> vertices;
    int rows = 8;
    int cols = 4;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float x0 = -0.8f + c * 0.4f;
            float x1 = x0 + 0.35f;
            float y0 = -0.8f + r * 0.2f;
            float y1 = y0 + 0.18f;

            // Сарғыш түсті үшбұрыш
            float oR = 0.9f, oG = 0.5f, oB = 0.2f;
            // Төменгі сарғыш үшбұрыш
            vertices.insert(vertices.end(), {
                x0, y0, oR, oG, oB,
                x1, y0, oR, oG, oB,
                x0, y1, oR, oG, oB
            });

            // Қара түсті үшбұрыш
            float bR = 0.1f, bG = 0.1f, bB = 0.1f;
            // Жоғарғы қара үшбұрыш
            vertices.insert(vertices.end(), {
                x1, y0, bR, bG, bB,
                x1, y1, bR, bG, bB,
                x0, y1, bR, bG, bB
            });
        }
    }

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // -----------------------------------------------------------------
    //  4. Негізгі цикл
    // -----------------------------------------------------------------
    double lastTime = glfwGetTime();
    int frameCount = 0;

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        frameCount++;
        // 4. Секундына 1 рет FPS шығару
        if (currentTime - lastTime >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }

        processInput(window);

        // --- 2 мен 3-Тапсырма: Фон түсін тазалау ---
        if (isSpacePressed) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Бос орын басылғанда ақ түс
        } else {
            float t = (float)glfwGetTime();
            // 2. Жиілік арттырылды (4.0f және 3.0f)
            float r = (std::sin(t * 4.0f) + 1.0f) * 0.5f * 0.3f;
            float g = (std::sin(t * 3.0f) + 1.0f) * 0.5f * 0.3f;
            glClearColor(r, g, 0.35f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        // Үшбұрыштарды салу
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 5);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // -----------------------------------------------------------------
    //  5. Тазалау
    // -----------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}