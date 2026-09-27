// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — 1-АПТА (ТОЛЫҚ ОРЫНДАЛҒАН ТАПСЫРМАЛАР)
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
// 1-ТАПСЫРМА: Терезенің өлшемін 1280x720 ету
const int WIDTH  = 1280;
const int HEIGHT = 720;

// 3-ТАПСЫРМА үшін жаһандық айнымалы
bool isSpacePressed = false;

// ---------------------------------------------------------------------
//  Терезе өлшемі өзгергенде шақырылады
// ---------------------------------------------------------------------
void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------
//  Пернетақтаны тексеру. Әр кадрда шақырылады.
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // 3-ТАПСЫРМА: Пробел басылғанын тексеру
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        isSpacePressed = true;
    } else {
        isSpacePressed = false;
    }
}

// =====================================================================
//  MAIN
// =====================================================================
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
    //  2. Терезе жасау (1280x720)
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

    // 4-ТАПСЫРМА: VSync өшіру (FPS шектеуін алып тастау)
    glfwSwapInterval(0);

    // -----------------------------------------------------------------
    //  3. GLAD: OpenGL функцияларын жүктеу
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    // 4-ТАПСЫРМА үшін айнымалылар (FPS есептеу)
    double lastTime = glfwGetTime();
    int frameCount = 0;

    // -----------------------------------------------------------------
    //  4. Негізгі цикл
    // -----------------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {

        // 3-ТАПСЫРМА: glClear-дан БҰРЫН енгізулерді өңдеу
        processInput(window);

        // 4-ТАПСЫРМА: FPS-ті секунд сайын консольге шығару
        double currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - lastTime >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }

        // --- Экранды тазалау ---
        if (isSpacePressed) {
            // 3-ТАПСЫРМА: Пробел басылса — ақ түс
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            // 2-ТАПСЫРМА: Түстің өзгеру жылдамдығын (жиілігін) арттыру
            // Жиілік t * 0.5f-тен t * 2.0f және t * 1.5f-ке өзгертілді
            float t = (float)glfwGetTime();
            float r = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
            float g = (std::sin(t * 1.5f) + 1.0f) * 0.5f * 0.3f;
            glClearColor(r, g, 0.35f, 1.0f);
        }

        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}