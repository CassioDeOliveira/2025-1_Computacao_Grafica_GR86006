// ==== IMPLEMENTAÇÃO DE BIBLIOTECAS EXTERNAS ====
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "Camera.h"

// ==== INCLUDES PADRÕES ====
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ==== STRUCT PARA ARMAZENAR UM VÉRTICE ====
struct Vertex {
    glm::vec3 pos, color, normal;
    glm::vec2 tex;
};

struct Model {
    GLuint VAO, VBO, textureID;
    size_t vertexCount;
    glm::vec3 ka, kd, ks;
    float shininess;
};


// ==== PROTÓTIPOS DE FUNÇÕES ====
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
int setupShader();
Model loadModel(const std::string& path);
void draw(GLuint shaderID, GLint modelLoc, GLint normalLoc, float angle, const std::vector<glm::vec3>& positions, const Model& model);

// ==== CONSTANTES GLOBAIS ====
const GLuint WIDTH = 1000, HEIGHT = 1000;
bool rotateX = false, rotateY = false, rotateZ = false;
glm::vec3 position(0.0f);   // posição
float scale = 1.0f;         // escala

// ==== OBJETOS OPENGL ====
GLuint VAO, VBO, textureID;
size_t vertexCount = 0;
glm::vec3 ka, kd, ks;
float shininess = 32.0f;
Camera camera(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = WIDTH / 2.0f, lastY = HEIGHT / 2.0f;
bool firstMouse = true;
float fov = 45.0f;

// ==== SHADERS ====
const GLchar* vertexShaderSource = R"(
#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;

out vec3 FragPos;
out vec3 Normal;
out vec4 finalColor;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main() {
    FragPos = vec3(model * vec4(position, 1.0));
    Normal = normalize(normalMatrix * normal);
    TexCoord = texCoord;
    finalColor = vec4(color, 1.0);
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

const GLchar* fragmentShaderSource = R"(
#version 450
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 finalColor;

out vec4 fragColor;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 ka, kd, ks;
uniform float shininess;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = ka * vec3(texture(texture1, TexCoord));
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = kd * diff * vec3(texture(texture1, TexCoord));
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = ks * spec;

    vec3 result = ambient + diffuse + specular;
    fragColor = vec4(result, 1.0) * finalColor;
}
)";

// ==== CARREGAMENTO DE MODELO OBJ COM TEXTURA ====
Model loadModel(const std::string& objPath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::filesystem::path baseDir = std::filesystem::path(objPath).parent_path();

    // Carrega o modelo com TinyObjLoader
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath.c_str(), baseDir.string().c_str());
    if (!ret) throw std::runtime_error(err);

    std::vector<Vertex> vertices;

    // Itera sobre os índices do modelo
    for (const auto& shape : shapes) {
        for (const auto& idx : shape.mesh.indices) {
            Vertex v;
            v.pos = {
                attrib.vertices[3 * idx.vertex_index + 0],
                attrib.vertices[3 * idx.vertex_index + 1],
                attrib.vertices[3 * idx.vertex_index + 2]
            };
            v.color = { 1.0f, 1.0f, 1.0f };
            v.tex = (idx.texcoord_index >= 0) ?
                glm::vec2(
                    attrib.texcoords[2 * idx.texcoord_index + 0],
                    attrib.texcoords[2 * idx.texcoord_index + 1]
                ) : glm::vec2(0.0f);

            v.normal = (idx.normal_index >= 0) ?
                glm::vec3(
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2]
                ) : glm::vec3(0.0f, 0.0f, 1.0f);
            vertices.push_back(v);
        }
    }

    vertexCount = vertices.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    // Carrega textura associada (se houver)
    if (!materials.empty()) {
        auto& mat = materials[0];
        ka = glm::make_vec3(mat.ambient);
        kd = glm::make_vec3(mat.diffuse);
        ks = glm::make_vec3(mat.specular);
        shininess = mat.shininess;

        if (!mat.diffuse_texname.empty()) {
            std::string texPath = (baseDir / mat.diffuse_texname).string();
            int w, h, channels;
            stbi_set_flip_vertically_on_load(true);
            unsigned char* data = stbi_load(texPath.c_str(), &w, &h, &channels, STBI_rgb_alpha);
            if (data) {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                stbi_image_free(data);
            } else {
                std::cerr << "Erro ao carregar textura: " << texPath << "\n";
            }
        }
    }

    Model model;
    model.VAO = VAO;
    model.VBO = VBO;
    model.textureID = textureID;
    model.vertexCount = vertexCount;
    model.ka = ka;
    model.kd = kd;
    model.ks = ks;
    model.shininess = shininess;
    return model;

}

// ==== FUNÇÃO PRINCIPAL ====
int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Textured Cube - Camera", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    lastX = WIDTH / 2.0f;
    lastY = HEIGHT / 2.0f;
    firstMouse = true;
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    GLuint shaderID = setupShader();
    Model cubeModel = loadModel("../assets/Modelos3D/Cube.obj");
    Model pkModel = loadModel("../assets/Modelos3D/Pumpkin.obj");

    glEnable(GL_DEPTH_TEST);
    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projLoc = glGetUniformLocation(shaderID, "projection");
    GLint normalLoc = glGetUniformLocation(shaderID, "normalMatrix");

    glUniform3f(glGetUniformLocation(shaderID, "lightPos"), 5.0f, 5.0f, 5.0f);
    glUniform3f(glGetUniformLocation(shaderID, "viewPos"), 0.0f, 0.0f, 10.0f);

    std::vector<glm::vec3> cubePositions = {
        glm::vec3(0.0f),
        glm::vec3(2.0f, 0.0f, 0.0f),
    };

    std::vector<glm::vec3> pkPositions = {
        glm::vec3(-2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 2.0f, 0.0f)
    };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderID, "viewPos"), 1, glm::value_ptr(camera.Position));

        float angle = (float)glfwGetTime();
        glActiveTexture(GL_TEXTURE0);
        
        draw(shaderID, modelLoc, normalLoc, angle, cubePositions, cubeModel);
        draw(shaderID, modelLoc, normalLoc, angle, pkPositions, pkModel);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

// ==== CALLBACK DE TECLADO PARA MOVIMENTO, ESCALA E ROTAÇÃO ====
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        float step = 0.1f;
        float scaleStep = 0.05f;

        // Rotação
        if (key == GLFW_KEY_X) rotateX = true, rotateY = rotateZ = false;
        if (key == GLFW_KEY_Y) rotateY = true, rotateX = rotateZ = false;
        if (key == GLFW_KEY_Z) rotateZ = true, rotateX = rotateY = false;

        // Movimento
        if (key == GLFW_KEY_UP) position.z -= step;
        if (key == GLFW_KEY_DOWN) position.z += step;
        if (key == GLFW_KEY_LEFT) position.x -= step;
        if (key == GLFW_KEY_RIGHT) position.x += step;
        if (key == GLFW_KEY_I) position.y += step;
        if (key == GLFW_KEY_J) position.y -= step;

        //VELOCIDADE DE MOVIMENTO
        if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL)  camera.MovementSpeed += 0.5f;
        if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) camera.MovementSpeed -= 0.5f;

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

// ==== CALLBACK DE MOUSE PARA CÂMERA ====
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// ==== CALLBACK DE SCROLL PARA ZOOM ====
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

// ==== FUNÇÃO DE DESENHO COM TRANSFORMAÇÃO ====
void draw(GLuint shaderID, GLint modelLoc, GLint normalLoc, float angle, const std::vector<glm::vec3>& positions, const Model& model) {
    glBindTexture(GL_TEXTURE_2D, model.textureID);
    glBindVertexArray(model.VAO);

    for (const auto& basePos : positions) {
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::scale(modelMat, glm::vec3(scale));

        if (rotateX) modelMat = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1, 0, 0)) * modelMat;
        if (rotateY) modelMat = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0)) * modelMat;
        if (rotateZ) modelMat = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1)) * modelMat;

        modelMat = glm::translate(modelMat, basePos + position);
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMat)));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix3fv(normalLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, glm::value_ptr(model.ka));
        glUniform3fv(glGetUniformLocation(shaderID, "kd"), 1, glm::value_ptr(model.kd));
        glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, glm::value_ptr(model.ks));
        glUniform1f(glGetUniformLocation(shaderID, "shininess"), model.shininess);

        glDrawArrays(GL_TRIANGLES, 0, model.vertexCount);
    }

    glBindVertexArray(0);
}

// ==== COMPILAÇÃO DOS SHADERS ====
int setupShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}
