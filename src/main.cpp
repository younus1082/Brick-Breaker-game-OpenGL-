#include "glad.h"
#include "glfw3.h"
#include <iostream>
#include <cmath>
#include <vector>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, float deltaTime);

// Window settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float SPEED_INCREASE_FACTOR = 1.40f; // 40% speed boost per hit

// Shader code
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec2 pos;
uniform float scale;
void main() {
    gl_Position = vec4(aPos.x * scale + pos.x, aPos.y * scale + pos.y, aPos.z, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
void main() {
    FragColor = vec4(color, 1.0);
}
)";

// Game objects
struct Brick {
    float x, y;
    bool alive;
    float r, g, b; // Color components
};

float paddleX = 0.0f;
const float paddleWidth = 0.25f;
float ballX = 0.0f, ballY = 0.0f;
// Initial speed increased by 60%
float ballSpeedX = 0.18f * 1.6f;
float ballSpeedY = 0.22f * 1.6f;
const float ballRadius = 0.05f;
int hitCount = 0;
std::vector<Brick> bricks;
bool gameOver = false;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Brick Breaker", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Shader setup
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

    // Initialize bricks (2 rows of 10)
    for(int row = 0; row < 2; row++) {
        for(int col = 0; col < 10; col++) {
            Brick brick;
            brick.x = -0.9f + col * 0.18f;  // x position (more compact to fit 10)
            brick.y = 0.8f - row * 0.15f;   // y position (moved higher)
            brick.alive = true;
            
            // Different colors for each row
            if (row == 0) {
                brick.r = 0.2f; 
                brick.g = 0.6f; 
                brick.b = 1.0f; // Blue
            } else {
                brick.r = 0.2f; 
                brick.g = 1.0f; 
                brick.b = 0.6f; // Green
            }
            
            bricks.push_back(brick);
        }
    }

    // Paddle vertices
    float paddleVertices[] = {
        -paddleWidth/2, -0.05f, 0.0f,
         paddleWidth/2, -0.05f, 0.0f,
         paddleWidth/2,  0.05f, 0.0f,
        -paddleWidth/2,  0.05f, 0.0f
    };

    unsigned int paddleVBO, paddleVAO;
    glGenVertexArrays(1, &paddleVAO);
    glGenBuffers(1, &paddleVBO);
    glBindVertexArray(paddleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, paddleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(paddleVertices), paddleVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Ball vertices (circle)
    const int numSegments = 40;
    float ballVertices[(numSegments + 2) * 3];
    ballVertices[0] = 0.0f;
    ballVertices[1] = 0.0f;
    ballVertices[2] = 0.0f;
    for (int i = 0; i <= numSegments; ++i) {
        float angle = 2.0f * M_PI * i / numSegments;
        ballVertices[3*(i+1)] = cosf(angle);
        ballVertices[3*(i+1)+1] = sinf(angle);
        ballVertices[3*(i+1)+2] = 0.0f;
    }
    unsigned int ballVBO, ballVAO;
    glGenVertexArrays(1, &ballVAO);
    glGenBuffers(1, &ballVBO);
    glBindVertexArray(ballVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ballVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ballVertices), ballVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float lastFrame = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        // Only update ball if game is not over
        if (!gameOver) {
            // Update ball position
            ballX += ballSpeedX * deltaTime;
            ballY += ballSpeedY * deltaTime;

            // Wall collisions
            if (ballX < -0.95f || ballX > 0.95f) ballSpeedX *= -1;
            if (ballY > 0.95f) ballSpeedY *= -1;
            
            // Check for ground collision (game over)
            if (ballY - ballRadius <= -1.0f) {
                gameOver = true;
                // Stop ball movement
                ballSpeedX = 0.0f;
                ballSpeedY = 0.0f;
            }

            // Paddle collision
            if (ballY < -0.8f && ballX > paddleX - paddleWidth/2 && ballX < paddleX + paddleWidth/2) {
                ballSpeedY = std::fabs(ballSpeedY);
                ballSpeedX = (ballX - paddleX) * 2.0f;
                hitCount++;
                if (hitCount <= 10) {
                    ballSpeedX *= SPEED_INCREASE_FACTOR;
                    ballSpeedY *= SPEED_INCREASE_FACTOR;
                }
            }

            // Brick collisions
            for (auto& brick : bricks) {
                if (brick.alive && 
                    ballX > brick.x - 0.08f && 
                    ballX < brick.x + 0.08f &&
                    ballY > brick.y - 0.05f &&
                    ballY < brick.y + 0.05f) {
                    brick.alive = false;
                    ballSpeedY *= -1;
                    break; // Only break one brick per frame
                }
            }
        }

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // Draw paddle
        glBindVertexArray(paddleVAO);
        glUniform2f(glGetUniformLocation(shaderProgram, "pos"), paddleX, -0.9f);
        glUniform1f(glGetUniformLocation(shaderProgram, "scale"), 1.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 0.5f, 0.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // Draw ball
        glBindVertexArray(ballVAO);
        glUniform2f(glGetUniformLocation(shaderProgram, "pos"), ballX, ballY);
        glUniform1f(glGetUniformLocation(shaderProgram, "scale"), ballRadius);
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 1.0f, 0.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, numSegments + 2);

        // Draw bricks
        glBindVertexArray(paddleVAO);
        for (const auto& brick : bricks) {
            if (brick.alive) {
                glUniform2f(glGetUniformLocation(shaderProgram, "pos"), brick.x, brick.y);
                glUniform1f(glGetUniformLocation(shaderProgram, "scale"), 0.16f); // Smaller scale to fit more bricks
                glUniform3f(glGetUniformLocation(shaderProgram, "color"), brick.r, brick.g, brick.b);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &paddleVAO);
    glDeleteBuffers(1, &paddleVBO);
    glDeleteVertexArrays(1, &ballVAO);
    glDeleteBuffers(1, &ballVBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, float deltaTime) {
    const float paddleSpeed = 1.2f;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        paddleX -= paddleSpeed * deltaTime;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        paddleX += paddleSpeed * deltaTime;
    
    // Fixed paddle movement limits to allow full movement to edges
    if (paddleX < -1.0f + paddleWidth/2) paddleX = -1.0f + paddleWidth/2;
    if (paddleX > 1.0f - paddleWidth/2) paddleX = 1.0f - paddleWidth/2;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
