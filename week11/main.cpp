#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

int gWidth = 800, gHeight = 600;
glm::vec3 camPos(0.0f, 0.0f, 3.0f);
glm::vec3 camFront(0.0f, 0.0f, -1.0f);
glm::vec3 camUp(0.0f, 1.0f, 0.0f);
float yaw = -90.0f, pitch = 0.0f, lastX = 400.0f, lastY = 300.0f;
float fov = 45.0f, uvScale = 1.0f;
bool firstMouse = true, invertY = false, wireframe = false;
GLuint textureId = 0;
GLenum wrapMode = GL_REPEAT;
GLenum magFilter = GL_LINEAR;
GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR;

const char* vertexSrc = R"(#version 330 core
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

const char* fragmentSrc = R"(#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D ourTexture;
uniform float uvScale;
void main() {
    FragColor = texture(ourTexture, vTexCoord * uvScale);
}
)";

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader error: " << log << '\n';
    }
    return shader;
}

GLuint createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Program link error: " << log << '\n';
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void applyTextureParameters() {
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    const float border[] = {1.0f, 0.0f, 0.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
}

GLuint loadTexture(const char* path) {
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    stbi_set_flip_vertically_on_load(true);
    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Image load failed: " << path << " (" << stbi_failure_reason() << ")\n";
        glDeleteTextures(1, &id);
        return 0;
    }
    GLenum format = channels == 4 ? GL_RGBA : (channels == 3 ? GL_RGB : GL_RED);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    std::cout << path << " " << width << "x" << height << " channels: " << channels << '\n';
    return id;
}

void framebufferCallback(GLFWwindow*, int width, int height) {
    gWidth = width;
    gHeight = height;
    glViewport(0, 0, width, height);
}

void mouseCallback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }
    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);
    if (invertY) yoffset = -yoffset;
    constexpr float sensitivity = 0.1f;
    yaw += xoffset * sensitivity;
    pitch = std::clamp(pitch + yoffset * sensitivity, -89.0f, 89.0f);
    glm::vec3 direction;
    direction.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    direction.y = std::sin(glm::radians(pitch));
    direction.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    camFront = glm::normalize(direction);
}

void scrollCallback(GLFWwindow*, double, double yoffset) {
    fov = std::clamp(fov - static_cast<float>(yoffset), 1.0f, 90.0f);
}

void setWrap(GLenum mode, const char* name) {
    wrapMode = mode;
    applyTextureParameters();
    std::cout << "Wrap: " << name << '\n';
}

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_1) setWrap(GL_REPEAT, "GL_REPEAT");
    if (key == GLFW_KEY_2) setWrap(GL_MIRRORED_REPEAT, "GL_MIRRORED_REPEAT");
    if (key == GLFW_KEY_3) setWrap(GL_CLAMP_TO_EDGE, "GL_CLAMP_TO_EDGE");
    if (key == GLFW_KEY_4) setWrap(GL_CLAMP_TO_BORDER, "GL_CLAMP_TO_BORDER");
    if (key == GLFW_KEY_N) {
        magFilter = GL_NEAREST;
        applyTextureParameters();
        std::cout << "MAG: GL_NEAREST\n";
    }
    if (key == GLFW_KEY_L) {
        magFilter = GL_LINEAR;
        applyTextureParameters();
        std::cout << "MAG: GL_LINEAR\n";
    }
    if (key == GLFW_KEY_M) {
        minFilter = minFilter == GL_LINEAR ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        applyTextureParameters();
        std::cout << "MIN: " << (minFilter == GL_LINEAR ? "GL_LINEAR" : "GL_LINEAR_MIPMAP_LINEAR") << '\n';
    }
    if (key == GLFW_KEY_I) {
        invertY = !invertY;
        std::cout << "Invert Y: " << invertY << '\n';
    }
    if (key == GLFW_KEY_TAB) wireframe = !wireframe;
    if (key == GLFW_KEY_R) {
        camPos = glm::vec3(0.0f, 0.0f, 3.0f);
        camFront = glm::vec3(0.0f, 0.0f, -1.0f);
        yaw = -90.0f; pitch = 0.0f; fov = 45.0f; uvScale = 1.0f;
    }
    if (key == GLFW_KEY_E) {
        while (glGetError() != GL_NO_ERROR) {}
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        GLenum error = glGetError();
        std::cout << "Intentional invalid MAG filter, glGetError = 0x" << std::hex << error << std::dec << '\n';
        applyTextureParameters();
    }
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "Week 11 - Textures", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGL(glfwGetProcAddress)) { glfwTerminate(); return -1; }

    // Position + UV. All faces keep the original CCW winding.
    const float vertices[] = {
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

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint program = createProgram();
    const GLint modelLoc = glGetUniformLocation(program, "model");
    const GLint viewLoc = glGetUniformLocation(program, "view");
    const GLint projectionLoc = glGetUniformLocation(program, "projection");
    const GLint scaleLoc = glGetUniformLocation(program, "uvScale");
    const std::string texturePath = std::string(ASSET_DIR) + "/container.png";
    textureId = loadTexture(texturePath.c_str());
    if (!textureId) { glfwTerminate(); return -1; }
    applyTextureParameters();

    const glm::vec3 positions[] = {
        {0,0,0}, {2,0.5f,-2}, {-1.8f,-0.3f,-1}, {1.2f,-0.8f,-3.5f}, {-2.2f,1,-4},
        {-5,0,-38}, {-3,1,-40}, {-1,-1,-42}, {1,0.5f,-39}, {3,-0.5f,-41}, {5,1,-40}
    };
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "ourTexture"), 0);
    std::cout << "Keys: 1-4 wrap, N/L MAG, M mipmap, [/] UV scale, E invalid MAG test, WASD/Space/Shift camera\n";

    float lastFrame = static_cast<float>(glfwGetTime());
    while (!glfwWindowShouldClose(window)) {
        const float now = static_cast<float>(glfwGetTime());
        const float dt = now - lastFrame;
        lastFrame = now;
        float speed = 2.5f * dt;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 2.0f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * camFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * camFront;
        const glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= speed * right;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += speed * right;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camPos += speed * camUp;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camPos -= speed * camUp;
        if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) uvScale += 2.0f * dt;
        if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) uvScale -= 2.0f * dt;
        uvScale = std::clamp(uvScale, 0.5f, 8.0f);

        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const float aspect = gHeight > 0 ? static_cast<float>(gWidth) / gHeight : 1.0f;
        const glm::mat4 projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 120.0f);
        const glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glUseProgram(program);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(scaleLoc, uvScale);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glBindVertexArray(vao);
        for (int i = 0; i < static_cast<int>(sizeof(positions) / sizeof(positions[0])); ++i) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), positions[i]);
            model = glm::rotate(model, now * (0.25f + i * 0.04f), glm::vec3(0.3f, 1.0f, 0.1f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteTextures(1, &textureId);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}
