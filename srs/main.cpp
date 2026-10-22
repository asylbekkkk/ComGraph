// Pong: заманауи OpenGL 3.3 Core (GLFW + GLAD 2 + GLM)
// Қолданылған тақырыптар: терезе/цикл (1), VAO/VBO (2), шейдерлер (3),
// uniform + delta time (4), EBO (5), GLM матрицалары (6), ортографиялық проекция (7)
// Әдепкі режим: екі ойыншы (W/S және Up/Down). Терезені созуға болады (4:3).

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

// Ойынның логикалық өлшемі (терезе қалай созылса да, ойын осы координатада жұмыс істейді)
const float W = 800.f, H = 600.f;
const float PADDLE_W = 14.f, PADDLE_H = 100.f, BALL = 14.f;
const float PADDLE_SPEED = 450.f, START_SPEED = 320.f, MAX_SPEED = 900.f;
const int WIN_SCORE = 4;

float lx = 30.f, ly = H / 2;       // сол ракетка (центрі)
float rx = W - 30.f, ry = H / 2;   // оң ракетка (центрі)
float bx = W / 2, by = H / 2;      // шар
float vx = 0, vy = 0;              // шар жылдамдығы
int scoreL = 0, scoreR = 0;
bool playing = false, gameOver = false;
bool vsAI = false;                 // false = екі ойыншы, true = оң жақта AI

// Шейдерлер 
const char* VERT_SRC = R"(#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 uModel;
uniform mat4 uProjection;
void main() {
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
}
)";

const char* FRAG_SRC = R"(#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        fprintf(stderr, "Шейдер қатесі: %s\n", log);
    }
    return s;
}

GLuint createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, VERT_SRC);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    int ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(p, 512, nullptr, log);
        fprintf(stderr, "Линковка қатесі: %s\n", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

GLint locModel = -1, locColor = -1;

// Бірлік төртбұрышты (VAO/EBO) масштабтап, жылжытып сызады
void drawRect(float cx, float cy, float w, float h, glm::vec3 color) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(cx, cy, 0.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));
    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(locColor, 1, glm::value_ptr(color));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

// 7 сегменттік цифр: a(үсті) b(оң үсті) c(оң асты) d(асты) e(сол асты) f(сол үсті) g(ортасы)
const bool SEG[10][7] = {
    {1,1,1,1,1,1,0}, {0,1,1,0,0,0,0}, {1,1,0,1,1,0,1}, {1,1,1,1,0,0,1},
    {0,1,1,0,0,1,1}, {1,0,1,1,0,1,1}, {1,0,1,1,1,1,1}, {1,1,1,0,0,0,0},
    {1,1,1,1,1,1,1}, {1,1,1,1,0,1,1}};

void drawDigit(int d, float x, float y, float h, glm::vec3 c) {
    float w = h * 0.6f, t = h * 0.14f;
    if (SEG[d][0]) drawRect(x, y + h / 2, w, t, c);
    if (SEG[d][1]) drawRect(x + w / 2, y + h / 4, t, h / 2, c);
    if (SEG[d][2]) drawRect(x + w / 2, y - h / 4, t, h / 2, c);
    if (SEG[d][3]) drawRect(x, y - h / 2, w, t, c);
    if (SEG[d][4]) drawRect(x - w / 2, y - h / 4, t, h / 2, c);
    if (SEG[d][5]) drawRect(x - w / 2, y + h / 4, t, h / 2, c);
    if (SEG[d][6]) drawRect(x, y, w, t, c);
}

void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    // ортаңғы үзік сызық
    for (float y = 15; y < H; y += 30) drawRect(W / 2, y, 4, 14, glm::vec3(0.35f, 0.35f, 0.4f));

    // ұпай
    drawDigit(scoreL, W / 2 - 100, H - 70, 60, glm::vec3(0.8f, 0.8f, 0.85f));
    drawDigit(scoreR, W / 2 + 100, H - 70, 60, glm::vec3(0.8f, 0.8f, 0.85f));

    // ракеткалар
    drawRect(lx, ly, PADDLE_W, PADDLE_H, glm::vec3(0.3f, 0.8f, 1.0f));
    drawRect(rx, ry, PADDLE_W, PADDLE_H, glm::vec3(1.0f, 0.4f, 0.5f));

    // шар: түсі уақытқа байланысты пульсация жасайды (uniform + time)
    float p = 0.75f + 0.25f * sinf((float)glfwGetTime() * 6.0f);
    drawRect(bx, by, BALL, BALL, glm::vec3(1.0f, p, p * 0.5f));
}

// ---------- Ойын логикасы ----------
void resetBall(int dir) {
    bx = W / 2; by = H / 2;
    float a = ((rand() % 100) / 100.f - 0.5f) * 0.8f;
    vx = dir * START_SPEED * cosf(a);
    vy = START_SPEED * sinf(a);
}

// ракеткаға тигенде: бұрыш тию орнына тәуелді, жылдамдық артады
void hitPaddle(float py, int dir) {
    float rel = glm::clamp((by - py) / (PADDLE_H / 2), -1.f, 1.f);
    float sp = std::min(std::hypot(vx, vy) * 1.07f, MAX_SPEED);
    float a = rel * 0.9f;
    vx = dir * sp * cosf(a);
    vy = sp * sinf(a);
}

void updateTitle(GLFWwindow* win) {
    char buf[200];
    const char* mode = vsAI ? "AI-ға қарсы" : "Екі ойыншы";
    if (gameOver)
        snprintf(buf, sizeof buf, "Pong | %s жеңді! SPACE - қайта бастау", scoreL > scoreR ? "Сол ойыншы" : "Оң ойыншы");
    else if (!playing)
        snprintf(buf, sizeof buf, "Pong | Режим: %s | SPACE - бастау | 1: AI, 2: екі ойыншы | W/S және Up/Down", mode);
    else
        snprintf(buf, sizeof buf, "Pong | %s", mode);
    glfwSetWindowTitle(win, buf);
}

void update(GLFWwindow* win, float dt) {
    auto key = [&](int k) { return glfwGetKey(win, k) == GLFW_PRESS; };

    if (!playing) {
        if (key(GLFW_KEY_1)) { vsAI = true;  updateTitle(win); }
        if (key(GLFW_KEY_2)) { vsAI = false; updateTitle(win); }
        if (key(GLFW_KEY_SPACE)) {
            if (gameOver) { scoreL = scoreR = 0; gameOver = false; }
            playing = true;
            resetBall(rand() % 2 ? 1 : -1);
            updateTitle(win);
        }
    }

    // сол ракетка: W/S (delta time арқылы кадр жиілігінен тәуелсіз)
    if (key(GLFW_KEY_W)) ly += PADDLE_SPEED * dt;
    if (key(GLFW_KEY_S)) ly -= PADDLE_SPEED * dt;

    // оң ракетка: AI немесе Up/Down
    if (vsAI) {
        float d = by - ry, step = PADDLE_SPEED * 0.65f * dt;
        if (playing && fabsf(d) > step) ry += d > 0 ? step : -step;
    } else {
        if (key(GLFW_KEY_UP))   ry += PADDLE_SPEED * dt;
        if (key(GLFW_KEY_DOWN)) ry -= PADDLE_SPEED * dt;
    }
    ly = glm::clamp(ly, PADDLE_H / 2, H - PADDLE_H / 2);
    ry = glm::clamp(ry, PADDLE_H / 2, H - PADDLE_H / 2);

    if (!playing) return;

    bx += vx * dt;
    by += vy * dt;

    // жоғарғы / төменгі қабырға
    if (by + BALL / 2 > H) { by = H - BALL / 2; vy = -vy; }
    if (by - BALL / 2 < 0) { by = BALL / 2;     vy = -vy; }

    // сол ракетка
    if (vx < 0 && bx - BALL / 2 < lx + PADDLE_W / 2 && bx + BALL / 2 > lx - PADDLE_W / 2 &&
        fabsf(by - ly) < PADDLE_H / 2 + BALL / 2) {
        bx = lx + PADDLE_W / 2 + BALL / 2;
        hitPaddle(ly, +1);
    }
    // оң ракетка
    if (vx > 0 && bx + BALL / 2 > rx - PADDLE_W / 2 && bx - BALL / 2 < rx + PADDLE_W / 2 &&
        fabsf(by - ry) < PADDLE_H / 2 + BALL / 2) {
        bx = rx - PADDLE_W / 2 - BALL / 2;
        hitPaddle(ry, -1);
    }

    // ұпай
    if (bx < 0 || bx > W) {
        int dir = bx < 0 ? -1 : 1;   // шар жеңілген жаққа қайта беріледі
        if (bx < 0) scoreR++; else scoreL++;
        if (scoreL >= WIN_SCORE || scoreR >= WIN_SCORE) {
            playing = false; gameOver = true;
            bx = W / 2; by = H / 2; vx = vy = 0;
            updateTitle(win);
        } else {
            resetBall(dir);
        }
    }
}

// Терезе өлшемі өзгергенде viewport-ты жаңарту
void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    srand((unsigned)time(nullptr));
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);   // терезені созуға болады

    GLFWwindow* win = glfwCreateWindow((int)W, (int)H, "Pong", nullptr, nullptr);
    if (!win) { fprintf(stderr, "Терезе құрылмады\n"); glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    if (!gladLoadGL(glfwGetProcAddress)) {
        fprintf(stderr, "GLAD іске қосылмады\n");
        return -1;
    }

    glfwSetFramebufferSizeCallback(win, framebufferSizeCallback);
    glfwSetWindowAspectRatio(win, 4, 3);          // 4:3 арақатынасы сақталады, сурет бұрмаланбайды
    glfwSetWindowSizeLimits(win, 400, 300, GLFW_DONT_CARE, GLFW_DONT_CARE);  // тым кіші болмасын

    int fbW, fbH;
    glfwGetFramebufferSize(win, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    GLuint program = createProgram();
    locModel = glGetUniformLocation(program, "uModel");
    locColor = glGetUniformLocation(program, "uColor");
    GLint locProj = glGetUniformLocation(program, "uProjection");

    // Бірлік төртбұрыш: 4 төбе + 6 индекс (EBO)
    float vertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f};
    unsigned int indices[] = {0, 1, 2, 2, 3, 0};

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Ортографиялық проекция: логикалық координаталар (0..W, 0..H).
    // Терезе созылғанда viewport өзгереді, сондықтан сурет өздігінен масштабталады.
    glm::mat4 projection = glm::ortho(0.0f, W, 0.0f, H, -1.0f, 1.0f);

    glClearColor(0.05f, 0.05f, 0.09f, 1.0f);
    updateTitle(win);

    double last = glfwGetTime();
    while (!glfwWindowShouldClose(win) && glfwGetKey(win, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
        double now = glfwGetTime();
        float dt = std::min((float)(now - last), 0.033f);
        last = now;

        update(win, dt);

        glUseProgram(program);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(vao);
        draw();

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}