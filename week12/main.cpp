// =====================================================================
//  12-апта: Бірнеше текстура. Текстуралық бірліктер. mix() арқылы араластыру.
//  (11-аптаның үстіне: екінші loadTexture, glActiveTexture x2, glUniform1i x2, blend, mix)
//
//  Суреттер (assets/): container.png, pattern.png, logo.png (4 арна, мөлдір фон)
//
//  БАСҚАРУ — камера (10-аптадағыдай)
//    Тінтуір — қарау, дөңгелек — зум, W S A D — жүру, Space / L-Shift — жоғары / төмен
//    L-Ctrl — жүгіру, R — қайтару, P — күйді шығару, I — Invert Y, Tab — тінтуірді босату
//
//  БАСҚАРУ — 12-апта
//    Up / Down — blend 0..1 (dt-мен, біртіндеп)
//    M         — араластыру режимі (терезе тақырыбында аты көрінеді):
//                0 mix(a,b,blend)            базалық бөлім
//                1 a * b                     3-тапсырма (көбейту)
//                2 a + b                     3-тапсырма (қосу)
//                3 clamp(a + b, 0, 1)        3-тапсырма (шектеумен)
//                4 mix(base, decal, a)       4-тапсырма: texture2 = logo.png
//                5 mix(a,b, vTexCoord.y)     5-тапсырма
//                6 mix(a,b, smoothstep)      5-тапсырма
//                7 үш текстура               Қ1
//    1         — 1-тапсырма: glActiveTexture екі жолын «комментарийге алу» (қосу/өшіру)
//    2         — 2-тапсырма: glUniform1i-ге бірлік емес, текстура ID-ін беру (қосу/өшіру)
//    B         — Қ2: blend уақытпен автоматты өзгереді (қосу/өшіру)
//    F         — Қ3: әр жаққа бөлек blend (6 рет сызу)
//    O         — Қ4: логотип айналады (қосу/өшіру)
//    ESC       — шығу
// =====================================================================
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <string>

#ifndef ASSET_DIR
#define ASSET_DIR "assets"
#endif

int gWidth = 800, gHeight = 600;

// ---------- камера (9-10 апта) ----------
const glm::vec3 CAM_START(0.0f, 0.0f, 3.0f);
glm::vec3 camPos   = CAM_START;
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camUp    = glm::vec3(0.0f, 1.0f, 0.0f);
const float YAW_START = -90.0f, FOV_START = 45.0f;
float yaw = YAW_START, pitch = 0.0f, lastX = 400.0f, lastY = 300.0f, fov = FOV_START;
bool  firstMouse = true, invertY = false, mouseOn = true;
const float sensitivity = 0.1f;

// ---------- 12-апта: күй ----------
float blend = 0.5f;                                   // 5-қадам: жаһандық коэффициент
int   mode = 0;                                       // 0..7 (жоғарыдағы тізім)
const char* MODE_NAMES[8] = { "mix(a,b,blend)", "a*b", "a+b", "clamp(a+b)", "decal (logo.a)",
                              "mix(y)", "smoothstep", "3 textures" };
bool brokenUnits = false;                             // 1-тапсырма
bool wrongID     = false;                             // 2-тапсырма
bool autoBlend   = false;                             // Қ2
bool perFace     = false;                             // Қ3
bool rotLogo     = false;                             // Қ4

void onResize(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
    gWidth = w;
    gHeight = (h > 0) ? h : 1;
}

void updateFront() {
    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camFront = glm::normalize(dir);
}

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (!mouseOn) return;
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    if (invertY) yoffset = -yoffset;
    lastX = (float)xpos;
    lastY = (float)ypos;
    yaw   += xoffset * sensitivity;
    pitch += yoffset * sensitivity;
    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    if (yaw >= 360.0f) yaw -= 360.0f;
    if (yaw <= -360.0f) yaw += 360.0f;
    updateFront();
}

void scroll_callback(GLFWwindow*, double, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 90.0f) fov = 90.0f;
}

bool pressedOnce(GLFWwindow* window, int key) {
    static bool prev[GLFW_KEY_LAST + 1] = {};
    bool now = glfwGetKey(window, key) == GLFW_PRESS;
    bool fired = now && !prev[key];
    prev[key] = now;
    return fired;
}

// ---------- 11-аптадағы loadTexture — ӨЗГЕРІССІЗ ----------
unsigned int loadTexture(const char* path) {
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);

    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (data) {
        GLenum fmt = (n == 4) ? GL_RGBA : GL_RGB;            // logo.png: n == 4 -> GL_RGBA
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << path << " " << w << "x" << h << " арна: " << n << std::endl;
    } else {
        std::cerr << "Сурет жүктелмеді: " << path << std::endl;
    }
    stbi_image_free(data);
    return id;
}

// ---------- шейдерлер ----------
const char* vsSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vTexCoord = aTexCoord;
}
)";

// 2-қадам: екі sampler + blend. Қалған режимдер — 3, 4, 5-тапсырмалар мен қосымшалар.
const char* fsSrc = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;     // тек Қ1 (үш текстура)
uniform float blend;
uniform int   mode;
uniform float time;
uniform int   rotLogo;

vec2 decalUV() {                // Қ4: логотипті айналдыру (UV-ді бұру)
    if (rotLogo == 0) return vTexCoord;
    vec2 c = vTexCoord - 0.5;
    float s = sin(time), co = cos(time);
    return vec2(c.x * co - c.y * s, c.x * s + c.y * co) + 0.5;
}

void main() {
    vec4 a = texture(texture1, vTexCoord);
    vec4 b = texture(texture2, vTexCoord);

    if (mode == 0)      FragColor = mix(a, b, blend);                 // базалық: сызықтық
    else if (mode == 1) FragColor = a * b;                            // көбейту
    else if (mode == 2) FragColor = a + b;                            // қосу
    else if (mode == 3) FragColor = clamp(a + b, 0.0, 1.0);           // қосу, шектеумен
    else if (mode == 4) {                                             // логотипті альфамен жапсыру
        vec4 decal = texture(texture2, decalUV());
        FragColor = mix(a, decal, decal.a);
    }
    else if (mode == 5) FragColor = mix(a, b, vTexCoord.y);           // кеңістіктік ауысу
    else if (mode == 6) FragColor = mix(a, b, smoothstep(0.3, 0.7, vTexCoord.y));
    else {                                                            // Қ1: үш текстура
        vec4 surface = mix(a, b, blend);
        vec4 decal = texture(texture3, decalUV());
        FragColor = mix(surface, decal, decal.a);
    }
}
)";

unsigned int compileShader(GLenum type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader қатесі: " << log << std::endl;
    }
    return shader;
}

unsigned int makeShader() {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    int success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "Линковка қатесі: " << log << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

int main() {
    if (!glfwInit()) { std::cerr << "GLFW iske qosylmady\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "Week 12 - two textures", nullptr, nullptr);
    if (!window) { std::cerr << "Tereze zhasalmady\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(1);
    if (gladLoadGL(glfwGetProcAddress) == 0) { std::cerr << "GLAD zhuktelmedi\n"; return -1; }
    std::cout << "GPU: " << glGetString(GL_RENDERER) << std::endl;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    updateFront();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // ---------- VAO / VBO ----------
    // 36 вершина: позиция (3 сан) + UV (2 сан). Бұл 11-аптадағы массив.
    float vertices[] = {
        -0.5f,-0.5f,-0.5f, 0,0,  -0.5f, 0.5f,-0.5f, 0,1,   0.5f, 0.5f,-0.5f, 1,1,
         0.5f, 0.5f,-0.5f, 1,1,   0.5f,-0.5f,-0.5f, 1,0,  -0.5f,-0.5f,-0.5f, 0,0,
        -0.5f,-0.5f, 0.5f, 0,0,   0.5f,-0.5f, 0.5f, 1,0,   0.5f, 0.5f, 0.5f, 1,1,
         0.5f, 0.5f, 0.5f, 1,1,  -0.5f, 0.5f, 0.5f, 0,1,  -0.5f,-0.5f, 0.5f, 0,0,
        -0.5f,-0.5f,-0.5f, 0,0,  -0.5f,-0.5f, 0.5f, 1,0,  -0.5f, 0.5f, 0.5f, 1,1,
        -0.5f, 0.5f, 0.5f, 1,1,  -0.5f, 0.5f,-0.5f, 0,1,  -0.5f,-0.5f,-0.5f, 0,0,
         0.5f,-0.5f,-0.5f, 0,0,   0.5f, 0.5f,-0.5f, 0,1,   0.5f, 0.5f, 0.5f, 1,1,
         0.5f, 0.5f, 0.5f, 1,1,   0.5f,-0.5f, 0.5f, 1,0,   0.5f,-0.5f,-0.5f, 0,0,
        -0.5f,-0.5f,-0.5f, 0,0,   0.5f,-0.5f,-0.5f, 1,0,   0.5f,-0.5f, 0.5f, 1,1,
         0.5f,-0.5f, 0.5f, 1,1,  -0.5f,-0.5f, 0.5f, 0,1,  -0.5f,-0.5f,-0.5f, 0,0,
        -0.5f, 0.5f,-0.5f, 0,0,  -0.5f, 0.5f, 0.5f, 0,1,   0.5f, 0.5f, 0.5f, 1,1,
         0.5f, 0.5f, 0.5f, 1,1,   0.5f, 0.5f,-0.5f, 1,0,  -0.5f, 0.5f,-0.5f, 0,0
    };
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    unsigned int shader = makeShader();

    // ---------- 1-қадам: екінші текстураны жүктеу ----------
    std::string p1 = std::string(ASSET_DIR) + "/container.png";
    std::string p2 = std::string(ASSET_DIR) + "/pattern.png";
    std::string p3 = std::string(ASSET_DIR) + "/logo.png";
    unsigned int tex1 = loadTexture(p1.c_str());
    unsigned int tex2 = loadTexture(p2.c_str());
    unsigned int tex3 = loadTexture(p3.c_str());            // 4-тапсырма және Қ1
    std::cout << "tex1 = " << tex1 << "      tex2 = " << tex2 << "      tex3 = " << tex3 << std::endl;

    // Қ4: логотип бұрылғанда шеті қайталанбасын — мөлдір шекара
    glBindTexture(GL_TEXTURE_2D, tex3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float clear[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clear);

    int locModel = glGetUniformLocation(shader, "model"), locView = glGetUniformLocation(shader, "view");
    int locProj = glGetUniformLocation(shader, "projection");
    int locBlend = glGetUniformLocation(shader, "blend"), locMode = glGetUniformLocation(shader, "mode");
    int locTime = glGetUniformLocation(shader, "time"), locRot = glGetUniformLocation(shader, "rotLogo");
    int locTexture1 = glGetUniformLocation(shader, "texture1");
    int locTexture2 = glGetUniformLocation(shader, "texture2");
    int locTexture3 = glGetUniformLocation(shader, "texture3");

    // ---------- 3-қадам: бірлік нөмірлері (glUseProgram-нан КЕЙІН!) ----------
    glUseProgram(shader);
    glUniform1i(locTexture1, 0);
    glUniform1i(locTexture2, 1);
    glUniform1i(locTexture3, 2);

    glm::vec3 cubePositions[] = {
        {0.0f, 0.0f, 0.0f}, {2.0f, 0.5f, -2.0f}, {-1.8f, -0.3f, -1.0f}, {1.2f, -0.8f, -3.5f}, {-2.2f, 1.0f, -4.0f}
    };

    std::string lastTitle;
    float last = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime(), dt = std::min(now - last, 0.05f);
        last = now;

        // ---------- енгізу ----------
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
        float speed = 2.5f * dt;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 2.0f;
        glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * camFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * camFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= right * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += right * speed;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camPos += speed * camUp;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camPos -= speed * camUp;
        if (glm::length(camPos) > 20.0f) camPos = glm::normalize(camPos) * 20.0f;

        // 5-қадам: blend — үздіксіз әрекет, сондықтан dt-ге көбейтіледі
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)   blend += 0.5f * dt;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) blend -= 0.5f * dt;
        if (autoBlend) blend = 0.5f + 0.5f * std::sin(now);                  // Қ2
        if (blend > 1.0f) blend = 1.0f;                                      // шектеу міндетті
        if (blend < 0.0f) blend = 0.0f;

        if (pressedOnce(window, GLFW_KEY_M)) { mode = (mode + 1) % 8; std::cout << "mode " << mode << ": " << MODE_NAMES[mode] << std::endl; }
        if (pressedOnce(window, GLFW_KEY_1)) {
            brokenUnits = !brokenUnits;
            // Бірліктерді таза күйге келтіреміз: 0-бірлікте tex1 қалсын, қалғандары бос
            for (int u = 2; u >= 1; --u) { glActiveTexture(GL_TEXTURE0 + u); glBindTexture(GL_TEXTURE_2D, 0); }
            glActiveTexture(GL_TEXTURE0);
            std::cout << "glActiveTexture жолдары: " << (brokenUnits ? "ӨШІРУЛІ (комментарийде)" : "қосулы") << std::endl;
        }
        if (pressedOnce(window, GLFW_KEY_2)) {
            wrongID = !wrongID;
            std::cout << "glUniform1i(texture2, " << (wrongID ? "tex2 ID = " + std::to_string(tex2) : std::string("1")) << ")" << std::endl;
        }
        if (pressedOnce(window, GLFW_KEY_B)) autoBlend = !autoBlend;
        if (pressedOnce(window, GLFW_KEY_F)) perFace = !perFace;
        if (pressedOnce(window, GLFW_KEY_O)) rotLogo = !rotLogo;
        if (pressedOnce(window, GLFW_KEY_I)) invertY = !invertY;
        if (pressedOnce(window, GLFW_KEY_R)) {
            camPos = CAM_START; yaw = YAW_START; pitch = 0.0f; fov = FOV_START; updateFront();
        }
        if (pressedOnce(window, GLFW_KEY_P)) {
            std::cout << std::fixed << std::setprecision(2) << "camPos = (" << camPos.x << ", " << camPos.y << ", " << camPos.z
                      << ")  yaw = " << yaw << "  pitch = " << pitch << "  fov = " << fov << "  blend = " << blend << std::endl;
        }
        if (pressedOnce(window, GLFW_KEY_TAB)) {
            mouseOn = !mouseOn;
            glfwSetInputMode(window, GLFW_CURSOR, mouseOn ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            if (mouseOn) firstMouse = true;
        }

        // ---------- сызу ----------
        glClearColor(0.07f, 0.08f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);

        // Қалыпты жағдайда sampler-ге 0, 1, 2 бірлік нөмірлері беріледі.
        // 2-тапсырмада wrongID=true болса, texture2 sampler-ге қате tex2 ID беріледі.
        glUniform1i(locTexture1, 0);
        glUniform1i(locTexture2, wrongID ? (int)tex2 : 1);
        glUniform1i(locTexture3, 2);

        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)gWidth / (float)gHeight, 0.1f, 100.0f);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        // 4-қадам: әр кадр байлау. glActiveTexture әрқашан glBindTexture-дан БҰРЫН.
        // 4-режимде (decal) 1-бірлікке pattern емес, logo қойылады (4-тапсырма: «екінші текстураны logo-ға ауыстыру»)
        unsigned int second = (mode == 4) ? tex3 : tex2;
        if (!brokenUnits) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, second);
            glActiveTexture(GL_TEXTURE2);                        // Қ1: үшінші бірлік (тек 7-режимде; әйтпесе бос)
            glBindTexture(GL_TEXTURE_2D, mode == 7 ? tex3 : 0);
            glActiveTexture(GL_TEXTURE0);
        } else {
            // 1-тапсырма: екі glActiveTexture «комментарийде» — екеуі де ағымдағы (0) бірлікке түседі
            glBindTexture(GL_TEXTURE_2D, tex1);
            glBindTexture(GL_TEXTURE_2D, second);
        }

        glUniform1f(locBlend, blend);                            // әр кадр — циклдің ішінде
        glUniform1i(locMode, mode);
        glUniform1f(locTime, now);
        glUniform1i(locRot, rotLogo ? 1 : 0);

        glBindVertexArray(VAO);
        for (int i = 0; i < 5; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            model = glm::rotate(model, now * (0.5f + 0.3f * i), glm::vec3(0.3f, 1.0f, 0.0f));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            if (perFace) {                                       // Қ3: әр жаққа өз blend-і (0, 0.2, ... 1)
                for (int f = 0; f < 6; ++f) {
                    glUniform1f(locBlend, f / 5.0f);
                    glDrawArrays(GL_TRIANGLES, f * 6, 6);
                }
            } else {
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        char buf[256];
        snprintf(buf, sizeof(buf), "mode %d: %s | blend %.2f%s%s%s%s", mode, MODE_NAMES[mode], blend,
                 brokenUnits ? " | [NO glActiveTexture]" : "", wrongID ? " | [WRONG ID]" : "",
                 autoBlend ? " | auto" : "", perFace ? " | per-face" : "");
        if (lastTitle != buf) { glfwSetWindowTitle(window, buf); lastTitle = buf; }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
