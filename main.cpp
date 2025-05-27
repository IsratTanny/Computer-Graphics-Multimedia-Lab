#include "glad.h"
#include "glfw3.h"

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

// Window size
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Launch control
bool launch = false;
bool launchedOnce = false;
float rocketY = -0.5f;

// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 model;
void main()
{
    gl_Position = model * vec4(aPos, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
void main()
{
    FragColor = vec4(color, 1.0);
}
)";

// Input
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !launchedOnce) {
        launch = true;
        launchedOnce = true;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Shader compilation
unsigned int createShaderProgram() {
    int success;
    char infoLog[512];

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "Vertex Shader Error:\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Fragment Shader Error:\n" << infoLog << std::endl;
    }

    unsigned int shaderProg = glCreateProgram();
    glAttachShader(shaderProg, vertexShader);
    glAttachShader(shaderProg, fragmentShader);
    glLinkProgram(shaderProg);
    glGetProgramiv(shaderProg, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProg, 512, NULL, infoLog);
        std::cout << "Shader Program Link Error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProg;
}

// Generate a simple circle (for planets, moon, window)
void generateCircle(std::vector<float>& vertices, float radius, int segments) {
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        vertices.push_back(radius * cos(angle));
        vertices.push_back(radius * sin(angle));
    }
}

// Set up VAO/VBO
unsigned int createShapeVAO(const std::vector<float>& vertices) {
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    return VAO;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Rocket Launch", NULL, NULL);
    if (!window) { std::cout << "Failed to create window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n"; return -1;
    }

    unsigned int shaderProgram = createShaderProgram();
    glUseProgram(shaderProgram);

    // Rocket body
    std::vector<float> rocketBody = {
        -0.05f, -0.2f,  0.05f, -0.2f,
         0.05f,  0.1f, -0.05f,  0.1f
    };
    unsigned int rocketBodyVAO = createShapeVAO(rocketBody);

    // Nose cone
    std::vector<float> nose = {
        -0.05f, 0.1f,  0.05f, 0.1f,  0.0f, 0.2f
    };
    unsigned int noseVAO = createShapeVAO(nose);

    // Left fin
    std::vector<float> finL = {
        -0.05f, -0.2f, -0.09f, -0.25f, -0.05f, -0.1f
    };
    unsigned int finLVAO = createShapeVAO(finL);

    // Right fin
    std::vector<float> finR = {
         0.05f, -0.2f,  0.09f, -0.25f,  0.05f, -0.1f
    };
    unsigned int finRVAO = createShapeVAO(finR);

    // Window
    std::vector<float> windowCircle;
    generateCircle(windowCircle, 0.02f, 20);
    unsigned int windowVAO = createShapeVAO(windowCircle);

    // Planet, moon
    std::vector<float> circle;
    generateCircle(circle, 1.0f, 40); // scalable
    unsigned int circleVAO = createShapeVAO(circle);

    // Fire flame
    std::vector<float> fire = {
        0.0f, -0.3f,
       -0.03f, -0.2f,
        0.03f, -0.2f
    };
    unsigned int fireVAO = createShapeVAO(fire);

    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int colorLoc = glGetUniformLocation(shaderProgram, "color");

    srand(42);
    std::vector<glm::vec2> stars;
    for (int i = 0; i < 200; ++i)
        stars.emplace_back((rand() % 2000 - 1000) / 1000.0f, (rand() % 2000 - 1000) / 1000.0f);

    glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw stars
        glUseProgram(shaderProgram);
        for (auto& star : stars) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(star, 0.0f));
            model = glm::scale(model, glm::vec3(0.002f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
            glBindVertexArray(circleVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 42);
        }

        // Draw moon
        glm::mat4 moon = glm::translate(glm::mat4(1.0f), glm::vec3(0.6f, 0.7f, 0.0f));
        moon = glm::scale(moon, glm::vec3(0.15f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(moon));
        glUniform3f(colorLoc, 0.9f, 0.9f, 0.9f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 42);

        // Planet 1
        glm::mat4 p1 = glm::translate(glm::mat4(1.0f), glm::vec3(-0.8f, 0.6f, 0.0f));
        p1 = glm::scale(p1, glm::vec3(0.1f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p1));
        glUniform3f(colorLoc, 0.0f, 1.0f, 1.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 42);

        // Planet 2
        glm::mat4 p2 = glm::translate(glm::mat4(1.0f), glm::vec3(0.8f, -0.5f, 0.0f));
        p2 = glm::scale(p2, glm::vec3(0.12f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p2));
        glUniform3f(colorLoc, 0.7f, 0.2f, 1.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 42);

        // Update rocketY if launched
        if (launch) rocketY += 0.00005f;//speed change
        glm::mat4 rocketBase = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, rocketY, 0.0f));

        // Rocket parts
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rocketBase));
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
        glBindVertexArray(rocketBodyVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        auto drawPart = [&](unsigned int vao, glm::vec3 color) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rocketBase));
            glUniform3f(colorLoc, color.r, color.g, color.b);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        };
        drawPart(noseVAO, {1.0f, 0.0f, 0.0f});
        drawPart(finLVAO, {1.0f, 0.0f, 0.0f});
        drawPart(finRVAO, {1.0f, 0.0f, 0.0f});

        // Window
        glm::mat4 windowTr = glm::translate(rocketBase, glm::vec3(0.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(windowTr));
        glUniform3f(colorLoc, 0.3f, 0.6f, 1.0f);
        glBindVertexArray(windowVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 22);

        // Fire
        if (launch) {
            float flicker = 0.98f + static_cast<float>(rand() % 5) / 100.0f;
            glm::mat4 fireTr = glm::translate(rocketBase, glm::vec3(0.0f, -0.05f, 0.0f));
            fireTr = glm::scale(fireTr, glm::vec3(flicker, flicker, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(fireTr));
            glUniform3f(colorLoc, 1.0f, 0.5f, 0.0f);
            glBindVertexArray(fireVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
