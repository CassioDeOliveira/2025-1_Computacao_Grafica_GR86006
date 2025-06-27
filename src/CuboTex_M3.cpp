// ==== IMPLEMENTAÇÃO DE BIBLIOTECAS EXTERNAS ====
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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
    glm::vec3 pos, color;
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
int setupShader();
Model loadModel(const std::string& path);
void draw(GLuint shaderID, GLint modelLoc, float angle, const std::vector<glm::vec3>& cubePositions, const Model& model);

// ==== CONSTANTES GLOBAIS ====
const GLuint WIDTH = 1000, HEIGHT = 1000;
bool rotateX = false, rotateY = false, rotateZ = false;
glm::vec3 position(0.0f);  // posição
float scale = 1.0f;        // escala 

// ==== OBJETOS OPENGL ====
GLuint VAO, VBO, textureID;
size_t vertexCount = 0;
glm::vec3 ka, kd, ks;
float shininess = 32.0f;

// ==== SHADERS ====
const GLchar* vertexShaderSource = R"(
    #version 450
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 color;
    layout(location = 2) in vec2 texCoord;

    out vec4 finalColor;
    out vec2 TexCoord;

    uniform mat4 model;

    void main() {
        gl_Position = model * vec4(position, 1.0);
        finalColor = vec4(color, 1.0);
        TexCoord = texCoord;
    }
)";

const GLchar* fragmentShaderSource = R"(
    #version 450
    in vec4 finalColor;
    in vec2 TexCoord;

    out vec4 fragColor;
    uniform sampler2D texture1;

    void main() {
        fragColor = texture(texture1, TexCoord) * finalColor;
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

            if (idx.texcoord_index >= 0) {
                v.tex = {
                    attrib.texcoords[2 * idx.texcoord_index + 0],
                    attrib.texcoords[2 * idx.texcoord_index + 1]
                };
            } else {
                v.tex = { 0.0f, 0.0f }; 
            }

            vertices.push_back(v);
        }
    }

    vertexCount = vertices.size();

    // Criação de VAO e VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    // Layout dos atributos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0); // posição
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color)); // cor
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex)); // textura
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Carrega textura associada (se houver)
    if (!materials.empty() && !materials[0].diffuse_texname.empty()) {
        std::string texPath = (baseDir / materials[0].diffuse_texname).string();
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
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Textured Cube - Cassio", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Compila shaders e carrega modelo
    GLuint shaderID = setupShader();
    Model cubeModel = loadModel("../assets/Modelos3D/Cube.obj");
    Model pkModel = loadModel("../assets/Modelos3D/Pumpkin.obj");

    glEnable(GL_DEPTH_TEST);
    glUseProgram(shaderID);

    // Vincula textura ao slot 0
    glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    // Posições na cena
    std::vector<glm::vec3> cubePositions = {
        glm::vec3(0.0f),
        glm::vec3(2.0f, 0.0f, 0.0f),
    };

    std::vector<glm::vec3> pkPositions = {
        glm::vec3(-2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 2.0f, 0.0f)
    };

    // Loop principal
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0, 0, 0, 1); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = (float)glfwGetTime();
        glActiveTexture(GL_TEXTURE0);
        
        draw(shaderID, modelLoc, angle, cubePositions, cubeModel);
        draw(shaderID, modelLoc, angle, pkPositions, pkModel);

        glfwSwapBuffers(window);
    }

    // Liberação de recursos
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

// ==== CALLBACK DE TECLADO PARA MOVIMENTO, ESCALA E ROTAÇÃO ====
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
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

// === FUNÇÃO DE DESENHO COM TRANSFORMAÇÃO ===
void draw(GLuint shaderID, GLint modelLoc, float angle, const std::vector<glm::vec3>& positions, const Model& model) {
    glBindTexture(GL_TEXTURE_2D, model.textureID);
    glBindVertexArray(model.VAO);

    for (const auto& basePos : positions) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(scale));

        if (rotateX) model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1, 0, 0)) * model;
        if (rotateY) model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0)) * model;
        if (rotateZ) model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1)) * model;

        model = glm::translate(model, basePos + position);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    glBindVertexArray(0);
}


// ==== COMPILAÇÃO DOS SHADERS ====
int setupShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
