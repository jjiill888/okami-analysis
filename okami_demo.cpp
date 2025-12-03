#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <vector>

const int WIDTH = 1280;
const int HEIGHT = 720;

int currentStage = 0;
const int MAX_STAGES = 5;
float rotationAngle = 0.0f;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.5f, 3.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);

// Mouse control variables
bool mousePressed = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;
float cameraAngleX = 0.0f;  // Horizontal rotation
float cameraAngleY = 0.0f;  // Vertical rotation
float cameraDistance = 3.0f;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

std::vector<Vertex> eggVertices;
std::vector<unsigned int> eggIndices;

void createEgg() {
    int stacks = 40;
    int slices = 40;

    for (int i = 0; i <= stacks; ++i) {
        float V = i / (float)stacks;
        float phi = V * M_PI;

        for (int j = 0; j <= slices; ++j) {
            float U = j / (float)slices;
            float theta = U * 2.0f * M_PI;

            float radiusScale = 0.6f + 0.2f * sin(phi);
            float x = radiusScale * cos(theta) * sin(phi);
            float y = 1.0f * cos(phi);
            float z = radiusScale * sin(theta) * sin(phi);

            Vertex vertex;
            vertex.position = glm::vec3(x, y, z);

            glm::vec3 normalDir = glm::vec3(
                x / (radiusScale * radiusScale),
                y / 1.0f,
                z / (radiusScale * radiusScale)
            );
            vertex.normal = glm::normalize(normalDir);
            eggVertices.push_back(vertex);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;

            eggIndices.push_back(first);
            eggIndices.push_back(second);
            eggIndices.push_back(first + 1);

            eggIndices.push_back(second);
            eggIndices.push_back(second + 1);
            eggIndices.push_back(first + 1);
        }
    }
}

const char* getVertexShader() {
    return R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        
        out vec3 FragPos;
        out vec3 Normal;
        out vec3 WorldPos;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            WorldPos = aPos;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";
}

const char* getFragmentShader(int stage) {
    switch(stage) {
        case 0: // Basic 3D
            return R"(
                #version 330 core
                in vec3 FragPos;
                in vec3 Normal;
                out vec4 FragColor;
                
                uniform vec3 lightPos;
                
                void main() {
                    vec3 norm = normalize(Normal);
                    vec3 lightDir = normalize(lightPos - FragPos);
                    float diff = max(dot(norm, lightDir), 0.0);
                    
                    vec3 ambient = vec3(0.3);
                    vec3 diffuse = diff * vec3(0.6);
                    vec3 result = (ambient + diffuse) * vec3(0.5, 0.5, 0.5);
                    
                    FragColor = vec4(result, 1.0);
                }
            )";
            
        case 1: // Cel shading - warm natural tones
            return R"(
                #version 330 core
                in vec3 FragPos;
                in vec3 Normal;
                out vec4 FragColor;
                
                uniform vec3 lightPos;
                
                void main() {
                    vec3 norm = normalize(Normal);
                    vec3 lightDir = normalize(lightPos - FragPos);
                    float diff = max(dot(norm, lightDir), 0.0);
                    
                    // Clear 3-tone cel shading
                    if (diff > 0.7) diff = 1.0;
                    else if (diff > 0.3) diff = 0.58;
                    else diff = 0.3;
                    
                    // Warm cream/ivory (natural, not harsh)
                    vec3 baseColor = vec3(0.96, 0.94, 0.87);
                    vec3 color = baseColor * diff;
                    
                    FragColor = vec4(color, 1.0);
                }
            )";
            
        case 2: // Sumi-e outlines - THICK and dramatic
            return R"(
                #version 330 core
                in vec3 FragPos;
                in vec3 Normal;
                out vec4 FragColor;
                
                uniform vec3 lightPos;
                uniform vec3 viewPos;
                
                void main() {
                    vec3 norm = normalize(Normal);
                    vec3 viewDir = normalize(viewPos - FragPos);
                    
                    // THICK edge detection
                    float edge = 1.0 - abs(dot(norm, viewDir));
                    edge = pow(edge, 1.7);
                    
                    // Cel shading
                    vec3 lightDir = normalize(lightPos - FragPos);
                    float diff = max(dot(norm, lightDir), 0.0);
                    if (diff > 0.7) diff = 1.0;
                    else if (diff > 0.3) diff = 0.58;
                    else diff = 0.3;
                    
                    vec3 baseColor = vec3(0.96, 0.94, 0.87);
                    vec3 color = baseColor * diff;
                    
                    // VERY THICK outlines (lower threshold)
                    if (edge > 0.24) {
                        color = vec3(0.07, 0.07, 0.07);
                    }
                    
                    FragColor = vec4(color, 1.0);
                }
            )";
            
        case 3: // Red tomoe - CONTROLLED patterns
            return R"(
                #version 330 core
                in vec3 FragPos;
                in vec3 Normal;
                in vec3 WorldPos;
                out vec4 FragColor;
                
                uniform vec3 lightPos;
                uniform vec3 viewPos;
                
                void main() {
                    vec3 norm = normalize(Normal);
                    vec3 viewDir = normalize(viewPos - FragPos);
                    
                    float edge = 1.0 - abs(dot(norm, viewDir));
                    edge = pow(edge, 1.7);
                    
                    vec3 lightDir = normalize(lightPos - FragPos);
                    float diff = max(dot(norm, lightDir), 0.0);
                    if (diff > 0.7) diff = 1.0;
                    else if (diff > 0.3) diff = 0.58;
                    else diff = 0.3;
                    
                    vec3 baseColor = vec3(0.96, 0.94, 0.87);
                    vec3 color = baseColor * diff;
                    
                    // CONTROLLED red patterns (not too much!)
                    float angle = atan(WorldPos.z, WorldPos.x);
                    float radius = length(vec2(WorldPos.x, WorldPos.z));
                    
                    // Main spiral (thinner, more selective)
                    float spiral = sin(angle * 3.5 - radius * 6.5);
                    
                    // Small circular accents
                    float spots = sin(WorldPos.y * 7.0) * cos(angle * 4.0);
                    
                    // MUCH higher thresholds = less red coverage
                    if (spiral > 0.82 || spots > 0.88) {
                        // Deeper red (not too bright)
                        vec3 redPattern = vec3(0.82, 0.12, 0.14);
                        color = mix(color, redPattern, 0.75);
                    }
                    
                    if (edge > 0.24) {
                        color = vec3(0.07, 0.07, 0.07);
                    }
                    
                    FragColor = vec4(color, 1.0);
                }
            )";
            
        case 4: // Full balanced style
            return R"(
                #version 330 core
                in vec3 FragPos;
                in vec3 Normal;
                in vec3 WorldPos;
                out vec4 FragColor;
                
                uniform vec3 lightPos;
                uniform vec3 viewPos;
                
                void main() {
                    vec3 norm = normalize(Normal);
                    vec3 viewDir = normalize(viewPos - FragPos);
                    
                    float edge = 1.0 - abs(dot(norm, viewDir));
                    edge = pow(edge, 1.7);
                    
                    // Richer cel shading (4 tones)
                    vec3 lightDir = normalize(lightPos - FragPos);
                    float diff = max(dot(norm, lightDir), 0.0);
                    if (diff > 0.8) diff = 1.0;
                    else if (diff > 0.5) diff = 0.7;
                    else if (diff > 0.25) diff = 0.45;
                    else diff = 0.25;
                    
                    // Warm base with subtle variation
                    vec3 baseColor = vec3(0.97, 0.95, 0.88);
                    vec3 color = baseColor * diff;
                    
                    // CONTROLLED red patterns
                    float angle = atan(WorldPos.z, WorldPos.x);
                    float radius = length(vec2(WorldPos.x, WorldPos.z));
                    float spiral = sin(angle * 3.5 - radius * 6.5);
                    float spots = sin(WorldPos.y * 7.0) * cos(angle * 4.0);
                    
                    if (spiral > 0.82 || spots > 0.88) {
                        vec3 redPattern = vec3(0.84, 0.11, 0.14);
                        color = mix(color, redPattern, 0.78);
                    }
                    
                    // Paper texture (subtle)
                    float noise1 = fract(sin(dot(FragPos.xy, vec2(12.9898, 78.233))) * 43758.5453);
                    float noise2 = fract(sin(dot(FragPos.yz, vec2(93.9898, 67.345))) * 28451.3547);
                    color += vec3((noise1 + noise2) * 0.02);
                    
                    // Subtle color variation
                    float variation = sin(WorldPos.y * 20.0) * 0.015;
                    color += vec3(variation, variation * 0.8, variation * 0.6);
                    
                    // Thick outlines with variation
                    if (edge > 0.24) {
                        float inkVar = noise1 * 0.05;
                        color = vec3(0.07 + inkVar);
                    }
                    
                    FragColor = vec4(color, 1.0);
                }
            )";
            
        default:
            return getFragmentShader(4);
    }
}

GLuint compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }
    
    return shader;
}

GLuint createShaderProgram(int stage) {
    GLuint vertexShader = compileShader(getVertexShader(), GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(getFragmentShader(stage), GL_FRAGMENT_SHADER);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Program linking failed: " << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

void updateCameraPosition() {
    // Calculate camera position based on angles
    float x = cameraDistance * cos(cameraAngleY) * sin(cameraAngleX);
    float y = cameraDistance * sin(cameraAngleY) + 0.5f;
    float z = cameraDistance * cos(cameraAngleY) * cos(cameraAngleX);
    
    cameraPos = glm::vec3(x, y, z);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mousePressed = true;
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        } else if (action == GLFW_RELEASE) {
            mousePressed = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (mousePressed) {
        double deltaX = xpos - lastMouseX;
        double deltaY = ypos - lastMouseY;
        
        // Update camera angles based on mouse movement
        cameraAngleX += deltaX * 0.005f;  // Horizontal rotation
        cameraAngleY -= deltaY * 0.005f;  // Vertical rotation (inverted)
        
        // Clamp vertical angle to prevent flipping
        if (cameraAngleY > M_PI / 2.0f - 0.1f) cameraAngleY = M_PI / 2.0f - 0.1f;
        if (cameraAngleY < -M_PI / 2.0f + 0.1f) cameraAngleY = -M_PI / 2.0f + 0.1f;
        
        updateCameraPosition();
        
        lastMouseX = xpos;
        lastMouseY = ypos;
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Zoom in/out with mouse wheel
    cameraDistance -= yoffset * 0.3f;
    
    // Clamp zoom distance
    if (cameraDistance < 1.5f) cameraDistance = 1.5f;
    if (cameraDistance > 10.0f) cameraDistance = 10.0f;
    
    updateCameraPosition();
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_SPACE || key == GLFW_KEY_RIGHT) {
            currentStage = (currentStage + 1) % (MAX_STAGES + 1);
            std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            std::cout << "Stage " << currentStage << ": ";
            switch(currentStage) {
                case 0: 
                    std::cout << "Basic 3D Model";
                    std::cout << "\n→ Standard Phong lighting";
                    break;
                case 1: 
                    std::cout << "Cel Shading (3-tone)";
                    std::cout << "\n→ Warm cream base color";
                    std::cout << "\n→ Clear light separation";
                    break;
                case 2: 
                    std::cout << "Sumi-e Outlines (THICK)";
                    std::cout << "\n→ VERY thick black lines";
                    std::cout << "\n→ Traditional brush stroke feel";
                    break;
                case 3: 
                    std::cout << "Tomoe Patterns (Controlled)";
                    std::cout << "\n→ Selective red markings";
                    std::cout << "\n→ Spiral + spot patterns";
                    break;
                case 4: 
                    std::cout << "Full Ōkami Style";
                    std::cout << "\n→ Paper texture";
                    std::cout << "\n→ Balanced colors";
                    std::cout << "\n→ Complete ukiyo-e aesthetic";
                    break;
                case 5: 
                    std::cout << "Animated (Auto-rotate)";
                    break;
            }
            std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        } else if (key == GLFW_KEY_LEFT) {
            currentStage = (currentStage - 1 + MAX_STAGES + 1) % (MAX_STAGES + 1);
        } else if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
            glfwSetWindowShouldClose(window, true);
        } else if (key == GLFW_KEY_R) {
            rotationAngle = 0.0f;
            cameraAngleX = 0.0f;
            cameraAngleY = 0.0f;
            cameraDistance = 3.0f;
            updateCameraPosition();
            std::cout << "Rotation and camera reset\n";
        }
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, 
        "Ōkami_Style_Demo", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    
    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    
    createEgg();
    
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, eggVertices.size() * sizeof(Vertex), 
                 &eggVertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, eggIndices.size() * sizeof(unsigned int), 
                 &eggIndices[0], GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   Ōkami - Balanced Amaterasu Style Demo                    ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "Controls:\n";
    std::cout << "  SPACE / →     : Next stage\n";
    std::cout << "  ←             : Previous stage\n";
    std::cout << "  R             : Reset rotation and camera\n";
    std::cout << "  ESC / Q       : Quit\n";
    std::cout << "  Mouse Drag    : Rotate camera view\n";
    std::cout << "  Mouse Wheel   : Zoom in/out\n\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "Stage 0: Basic 3D Model\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        glClearColor(0.94f, 0.91f, 0.83f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        static int lastStage = -1;
        static GLuint shaderProgram = 0;
        if (currentStage != lastStage) {
            if (shaderProgram != 0) {
                glDeleteProgram(shaderProgram);
            }
            shaderProgram = createShaderProgram(currentStage < MAX_STAGES ? currentStage : 4);
            lastStage = currentStage;
        }
        
        glUseProgram(shaderProgram);
        
        if (currentStage == MAX_STAGES) {
            rotationAngle += 0.008f;
        }
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                               (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 
                          1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 
                          1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 
                          1, GL_FALSE, glm::value_ptr(projection));
        
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 
                    1, glm::value_ptr(glm::vec3(3.0f, 3.0f, 3.0f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 
                    1, glm::value_ptr(cameraPos));
        glUniform1f(glGetUniformLocation(shaderProgram, "time"), 
                   (float)glfwGetTime());
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, eggIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        glfwSwapBuffers(window);
    }
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    
    glfwTerminate();
    
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║            Demo closed. Thanks for watching!                ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}