// Trajetória - Cassio

#include "Camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

struct Trajectory {
    std::vector<glm::vec3> controlPoints;
    size_t currentIndex = 0;
    float moveSpeed = 1.0f;
    glm::vec3 currentPos = glm::vec3(0.0f);
};


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
Model loadModel(const std::string& path);
void loadTrajectoriesFromTxt(const std::string& path);
void saveTrajectoriesToTxt(const std::string& path);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
int intersectedObjectIndex(const glm::vec3& rayOrigin, const glm::vec3& rayDir);
glm::vec3 calculateRayDirection(const glm::mat4& projection, const glm::mat4& view);
int setupShader();

const GLuint WIDTH = 1000, HEIGHT = 1000;
bool rotateX = false, rotateY = false, rotateZ = false;
glm::vec3 position(0.0f);
float scale = 1.0f;

GLuint VAO, VBO, textureID;
size_t vertexCount = 0;

glm::vec3 ka(0.2f), kd(0.8f), ks(1.0f);
float shininess = 32.0f;

Camera camera(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
float deltaTime = 0.0f;        
float animationDelta = 0.0f;   
float lastFrame = 0.0f;
float lastX = WIDTH / 2.0f, lastY = HEIGHT / 2.0f;
bool firstMouse = true;
float fov = 45.0f;
bool isPaused = false;
std::vector<Trajectory> trajectories;
size_t selectedObject = 0;
int highlightedObject = -1; 

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



int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Textured Cube - Track", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    lastX = WIDTH / 2.0f;
    lastY = HEIGHT / 2.0f;
    firstMouse = true;
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    GLuint shaderID = setupShader();
    std::vector<Model> models;
    models.push_back(loadModel("../assets/Modelos3D/CastleRuins.obj"));
    models.push_back(loadModel("../assets/Modelos3D/Pumpkin.obj")); 


    glEnable(GL_DEPTH_TEST);
    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projLoc = glGetUniformLocation(shaderID, "projection");
    GLint normalLoc = glGetUniformLocation(shaderID, "normalMatrix");

    glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, glm::value_ptr(ka));
    glUniform3fv(glGetUniformLocation(shaderID, "kd"), 1, glm::value_ptr(kd));
    glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, glm::value_ptr(ks));
    glUniform1f(glGetUniformLocation(shaderID, "shininess"), shininess);
    glUniform3f(glGetUniformLocation(shaderID, "lightPos"), 5.0f, 5.0f, 5.0f);
    glUniform3f(glGetUniformLocation(shaderID, "viewPos"), 0.0f, 0.0f, 10.0f);

    std::vector<glm::vec3> objectPositions = {
    glm::vec3(0.0f),
    glm::vec3(5.0f, 6.0f, 5.0f),
    };


    trajectories.resize(objectPositions.size());
    for (size_t i = 0; i < objectPositions.size(); ++i) {
        trajectories[i].currentPos = objectPositions[i];
    }


    loadTrajectoriesFromTxt("../Trajectories/trajectories2.txt");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        animationDelta = isPaused ? 0.0f : deltaTime;

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

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBindVertexArray(VAO);

        for (size_t i = 0; i < objectPositions.size(); ++i) {
            auto& model = models[i];
            auto& traj = trajectories[i];

            // Atualização da trajetória
            if (!traj.controlPoints.empty()) {
                glm::vec3 target = traj.controlPoints[traj.currentIndex];
                glm::vec3 direction = glm::normalize(target - traj.currentPos);
                float distance = glm::distance(target, traj.currentPos);
                float step = traj.moveSpeed * animationDelta;

                if (step >= distance) {
                    traj.currentPos = target;
                    traj.currentIndex = (traj.currentIndex + 1) % traj.controlPoints.size();
                } else {
                    traj.currentPos += direction * step;
                }
            } else {
                traj.currentPos = objectPositions[i];
            }

            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, traj.currentPos + position);
            modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glUniformMatrix3fv(normalLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

            glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, glm::value_ptr(model.ka));
            glUniform3fv(glGetUniformLocation(shaderID, "kd"), 1, glm::value_ptr(model.kd));
            glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, glm::value_ptr(model.ks));
            glUniform1f(glGetUniformLocation(shaderID, "shininess"), model.shininess);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, model.textureID);
            glBindVertexArray(model.VAO);

            glDrawArrays(GL_TRIANGLES, 0, model.vertexCount);

            // Highlight 
            if (i == highlightedObject) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(2.0f);
                glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, glm::value_ptr(glm::vec3(1.0f, 0.0f, 0.0f)));
                glUniform3fv(glGetUniformLocation(shaderID, "kd"), 1, glm::value_ptr(glm::vec3(1.0f, 0.0f, 0.0f)));
                glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, glm::value_ptr(glm::vec3(0.0f)));
                glUniform1f(glGetUniformLocation(shaderID, "shininess"), 1.0f);
                glDrawArrays(GL_TRIANGLES, 0, model.vertexCount);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                // Restaurar material original
                glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, glm::value_ptr(model.ka));
                glUniform3fv(glGetUniformLocation(shaderID, "kd"), 1, glm::value_ptr(model.kd));
                glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, glm::value_ptr(model.ks));
                glUniform1f(glGetUniformLocation(shaderID, "shininess"), model.shininess);
            }
        }   
  

        glBindVertexArray(0);
        glfwSwapBuffers(window);

    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

Model loadModel(const std::string& objPath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::filesystem::path baseDir = std::filesystem::path(objPath).parent_path();
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath.c_str(), baseDir.string().c_str());

    if (!ret) throw std::runtime_error(err);

    std::vector<Vertex> vertices;

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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        
        if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL)  camera.MovementSpeed += 0.5f;
        if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) camera.MovementSpeed -= 0.5f;
        if (key == GLFW_KEY_LEFT_BRACKET) scale -= 0.05f;
        if (key == GLFW_KEY_RIGHT_BRACKET) scale += 0.05f;
        if (key == GLFW_KEY_R) scale = 1.0f;
    }
    
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    isPaused = !isPaused;
    std::cout << (isPaused ? "Animação pausada.\n" : "Animação retomada.\n");
    }

    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        glm::vec3 current = camera.Position;
        trajectories[selectedObject].controlPoints.push_back(current);
        std::cout << "Ponto adicionado para objeto " << selectedObject << ": (" << current.x << ", " << current.y << ", " << current.z << ")\n";
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    loadTrajectoriesFromTxt("../Trajectories/trajectories.txt");

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    saveTrajectoriesToTxt("../Trajectories/trajectories.txt");
}

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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        glm::vec3 rayDir = calculateRayDirection(projection, view);
        glm::vec3 rayOrigin = camera.Position;

        int hit = intersectedObjectIndex(rayOrigin, rayDir);
        if (hit != -1) {
    if (selectedObject == hit) {
        selectedObject = -1;
        highlightedObject = -1;
    } else {
        selectedObject = hit;
        highlightedObject = hit;
        std::cout << "Objeto " << hit << " selecionado com o mouse.\n";
    }
}
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

void loadTrajectoriesFromTxt(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir " << path << " para leitura.\n";
        return;
    }

    std::string line;
    size_t currentObject = 0;
    trajectories.clear();
    trajectories.resize(4); 

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line[0] == '#') {
            sscanf(line.c_str(), "# Objeto %zu", &currentObject);
            if (currentObject >= trajectories.size())
                trajectories.resize(currentObject + 1);
        } else {
            float x, y, z;
            if (sscanf(line.c_str(), "%f %f %f", &x, &y, &z) == 3) {
                trajectories[currentObject].controlPoints.emplace_back(x, y, z);
            }
        }
    }

    std::cout << "Trajetórias carregadas de " << path << "\n";
}

void saveTrajectoriesToTxt(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir " << path << " para escrita.\n";
        return;
    }

    for (size_t i = 0; i < trajectories.size(); ++i) {
        file << "# Objeto " << i << "\n";
        for (const auto& p : trajectories[i].controlPoints)
            file << p.x << " " << p.y << " " << p.z << "\n";
        file << "\n";
    }

    std::cout << "Trajetórias salvas em " << path << "\n";
}

glm::vec3 calculateRayDirection(const glm::mat4& projection, const glm::mat4& view) {
    glm::vec2 ndc = glm::vec2(0.0f); 
    glm::vec4 clipCoords = glm::vec4(ndc, -1.0f, 1.0f);
    glm::vec4 eyeCoords = glm::inverse(projection) * clipCoords;
    eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);
    glm::vec3 worldRay = glm::normalize(glm::vec3(glm::inverse(view) * eyeCoords));
    return worldRay;
}


int intersectedObjectIndex(const glm::vec3& rayOrigin, const glm::vec3& rayDir) {
    for (size_t i = 0; i < trajectories.size(); ++i) {
        glm::vec3 center = trajectories[i].currentPos + position;
        float halfSize = scale * 0.5f;

        glm::vec3 minBox = center - glm::vec3(halfSize);
        glm::vec3 maxBox = center + glm::vec3(halfSize);

        // AABB ray-box intersection 
        float tmin = (minBox.x - rayOrigin.x) / rayDir.x;
        float tmax = (maxBox.x - rayOrigin.x) / rayDir.x;
        if (tmin > tmax) std::swap(tmin, tmax);

        float tymin = (minBox.y - rayOrigin.y) / rayDir.y;
        float tymax = (maxBox.y - rayOrigin.y) / rayDir.y;
        if (tymin > tymax) std::swap(tymin, tymax);

        if ((tmin > tymax) || (tymin > tmax)) continue;

        tmin = std::max(tmin, tymin);
        tmax = std::min(tmax, tymax);

        float tzmin = (minBox.z - rayOrigin.z) / rayDir.z;
        float tzmax = (maxBox.z - rayOrigin.z) / rayDir.z;
        if (tzmin > tzmax) std::swap(tzmin, tzmax);

        if ((tmin > tzmax) || (tzmin > tmax)) continue;

        return (int)i;
    }
    return -1;
}

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