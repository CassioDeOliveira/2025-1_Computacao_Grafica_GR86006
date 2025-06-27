// Hello Cube - Cassio

// ==== INCLUDES ====
#include <iostream>
#include <vector>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ==== CONSTANTES GLOBAIS ====
const GLuint WIDTH = 1000, HEIGHT = 1000;
bool rotateX = false, rotateY = false, rotateZ = false;
glm::vec3 position(0.0f);  // Translação
float scale = 1.0f;        // Escala

// ==== PROTÓTIPOS ====
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
int setupGeometry();

// ==== SHADERS ====
const GLchar* vertexShaderSource = R"glsl(
    #version 450
    layout (location = 0) in vec3 position;
    layout (location = 1) in vec3 color;

    uniform mat4 model;

    out vec4 finalColor;

    void main() {
        gl_Position = model * vec4(position, 1.0);
        finalColor = vec4(color, 1.0);
    }
)glsl";

const GLchar* fragmentShaderSource = R"glsl(
    #version 450
    in vec4 finalColor;
    out vec4 color;

    void main() {
        color = finalColor;
    }
)glsl";

// ==== FUNÇÃO PRINCIPAL ====
int main() {
    // Inicializa GLFW
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Cubo 3D - Cassio", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Compila shaders e configura geometria
    GLuint shaderID = setupShader();
    GLuint VAO = setupGeometry();

    glEnable(GL_DEPTH_TEST);
    glUseProgram(shaderID);

    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    // Posições iniciais dos cubos
    std::vector<glm::vec3> cubePositions = {
        glm::vec3(0.0f),
        glm::vec3(2.0f, 0.0f, 0.0f),
        glm::vec3(-2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 2.0f, 0.0f)
    };

    // Loop principal
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = (GLfloat)glfwGetTime();

        glBindVertexArray(VAO);

        for (auto basePos : cubePositions) {
            glm::mat4 model = glm::mat4(1.0f);

            // Escala
            model = glm::scale(model, glm::vec3(scale));

            // Rotações
            if (rotateX) model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1, 0, 0)) * model;
            if (rotateY) model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0)) * model;
            if (rotateZ) model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1)) * model;

            // Translação
            model = glm::translate(model, basePos + position);

            // Envia matriz para shader
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Desenha cubo 
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    // Liberação de recursos
    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

// ==== CALLBACK DE TECLADO ====
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        float step = 0.1f;
        float scaleStep = 0.05f;

        // Rotações
        if (key == GLFW_KEY_X) rotateX = true, rotateY = rotateZ = false;
        if (key == GLFW_KEY_Y) rotateY = true, rotateX = rotateZ = false;
        if (key == GLFW_KEY_Z) rotateZ = true, rotateX = rotateY = false;

        // Translação
        if (key == GLFW_KEY_W) position.z -= step;
        if (key == GLFW_KEY_S) position.z += step;
        if (key == GLFW_KEY_A) position.x -= step;
        if (key == GLFW_KEY_D) position.x += step;
        if (key == GLFW_KEY_I) position.y += step;
        if (key == GLFW_KEY_J) position.y -= step;

        // Escala
        if (key == GLFW_KEY_LEFT_BRACKET)  scale -= scaleStep;
        if (key == GLFW_KEY_RIGHT_BRACKET) scale += scaleStep;

        // Reset
        if (key == GLFW_KEY_R) {
            position = glm::vec3(0.0f);
            scale = 1.0f;
            rotateX = rotateY = rotateZ = false;
        }
    }
}

// ==== COMPILAÇÃO E LINKAGEM DOS SHADERS ====
int setupShader() {
    // Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Programa final
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Limpeza
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// ==== CONFIGURAÇÃO DOS VÉRTICES DO CUBO ====
int setupGeometry() {
    // Cada face do cubo tem 2 triângulos 
    GLfloat vertices[] = {
        // Frente
        -0.5f,-0.5f, 0.5f, 1,0,0,   0.5f,-0.5f, 0.5f, 1,0,0,   0.5f, 0.5f, 0.5f, 1,0,0,
        -0.5f,-0.5f, 0.5f, 0.5f,0.5f,0.5f,   0.5f, 0.5f, 0.5f, 0.5f,0.5f,0.5f,   -0.5f, 0.5f, 0.5f, 0.5f,0.5f,0.5f,
        // Traseira
        -0.5f,-0.5f,-0.5f, 0,0,1,   0.5f, 0.5f,-0.5f, 0,0,1,   0.5f,-0.5f,-0.5f, 0,0,1,
        -0.5f,-0.5f,-0.5f, 1,1,0,   -0.5f, 0.5f,-0.5f, 1,1,0,   0.5f, 0.5f,-0.5f, 1,1,0,
        // Esquerda
        -0.5f,-0.5f,-0.5f, 1,0,1,   -0.5f,-0.5f, 0.5f, 1,0,1,   -0.5f, 0.5f, 0.5f, 1,0,1,
        -0.5f,-0.5f,-0.5f, 0,1,1,   -0.5f, 0.5f, 0.5f, 0,1,1,   -0.5f, 0.5f,-0.5f, 0,1,1,
        // Direita
         0.5f,-0.5f,-0.5f, 0.6,0,0.6,   0.5f, 0.5f, 0.5f, 0.6,0,0.6,   0.5f,-0.5f, 0.5f, 0.6,0,0.6,
         0.5f,-0.5f,-0.5f, 0,0,0,      0.5f, 0.5f,-0.5f, 0,0,0,       0.5f, 0.5f, 0.5f, 0,0,0,
        // Superior
        -0.5f, 0.5f,-0.5f, 0.5,0,0.5,   -0.5f, 0.5f, 0.5f, 0.5,0,0.5,   0.5f, 0.5f, 0.5f, 0.5,0,0.5,
        -0.5f, 0.5f,-0.5f, 0.3,0.7,0.2,  0.5f, 0.5f, 0.5f, 0.3,0.7,0.2, 0.5f, 0.5f,-0.5f, 0.3,0.7,0.2,
        // Inferior
        -0.5f,-0.5f,-0.5f, 0.9,0.5,0.2,  0.5f,-0.5f, 0.5f, 0.9,0.5,0.2, -0.5f,-0.5f, 0.5f, 0.9,0.5,0.2,
        -0.5f,-0.5f,-0.5f, 0.1,0.8,0.9,  0.5f,-0.5f,-0.5f, 0.1,0.8,0.9, 0.5f,-0.5f, 0.5f, 0.1,0.8,0.9
    };

    // Geração e configuração de buffers
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Atributo posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Atributo cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}
