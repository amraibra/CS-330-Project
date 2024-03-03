/* Amra Ibrahimovic
   CS-330
   Professor Rodriguez
   February 25, 2024 */

#include <iostream> 
#include <cstdlib>              // EXIT_FAILURE
#include <GL/glew.h>            // GLEW library
#include <GLFW/glfw3.h>         // GLFW library

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> // Image loading Utility functions

// Camera header (in includes file)
#include <learnOpengl/camera.h> 
#include <vector>

using namespace std;
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

namespace
{
    const char* const WINDOW_TITLE = "Final Project";

    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;         // Handle for the vertex buffer object
        GLuint ebo;
        GLuint nVertices;    // Number of indices of the meshA
    };

    GLFWwindow* gWindow = nullptr;

    // Mesh 
    GLMesh gMesh;
    GLMesh planeMesh;
    GLMesh sphereMesh;
    GLMesh bookMesh;
    GLMesh pumpkinMesh;
    GLMesh ellipticalCylinderMesh;
    GLMesh cylinderMesh;

    // Texture
    GLuint gTextureId;
    GLuint cylinderTextureId;
    GLuint sphereTextureId;
    GLuint pumpkinTextureId;
    GLuint bookTextureId;
    GLuint perfumeTextureId;

    // Shader program
    GLuint gProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 7.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Booleans for "P" press
    bool isPerspective = true;
    bool pKeyWasPressed = false;

}

bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateSphere(GLMesh& mesh, float radius, int sectorCount, int stackCount); // Sphere
void UCreatePlane(GLMesh& mesh); // Plane
void UCreateBook(GLMesh& mesh);
void UCreateEllipticalCylinder(GLMesh& mesh, float baseRadius, float topRadius, float height, int sectorCount);
void UCreateCylinder(GLMesh& mesh, float baseRadius, float topRadius, float height, int sectorCount);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

// Vertext shader
const GLchar* vertexShaderSource = GLSL(440,
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 vertexTextureCoordinate;

// Global variables for the transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(position, 1.0));
    Normal = mat3(transpose(inverse(model))) * normal;
    vertexTextureCoordinate = texCoord; 
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
);

const GLchar* fragmentShaderSource = GLSL(440,
in vec3 FragPos;
in vec3 Normal;
in vec2 vertexTextureCoordinate; // Texture

out vec4 fragmentColor;

uniform sampler2D uTexture;
uniform vec3 ambientLightColor;
uniform vec3 diffuseLightColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform bool useTexture;

void main()
{

    // Ambient 
    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * ambientLightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * diffuseLightColor;

    // Combine
    vec4 texColor = texture(uTexture, vertexTextureCoordinate);
    vec3 result = (ambient + diffuse) * texColor.rgb;

    fragmentColor = vec4(result, texColor.a);
}
);

// Flips image
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Sphere 
    float sphereRadius = 0.15f;
    int sectorCount = 36;
    int stackCount = 18;
    UCreateSphere(sphereMesh, sphereRadius, sectorCount, stackCount);

    // Plane
    UCreatePlane(planeMesh);

    // Book
    UCreateBook(bookMesh);

    // Pumpkin
    float pumpkinRadius = 0.45f;
    int pumpkinSectorCount = 36;
    int pumpkinStackCount = 18;
    UCreateSphere(pumpkinMesh, pumpkinRadius, pumpkinSectorCount, pumpkinStackCount);

    // Perfume
    float baseRadius = 0.2f;
    float topRadius = 0.2f;
    float perfumeHeight = 0.7f;
    int perfumeSectorCount = 36;
    UCreateEllipticalCylinder(ellipticalCylinderMesh, baseRadius, topRadius, perfumeHeight, perfumeSectorCount);

    // Candle
    float cylinderBaseRadius = 0.4f;
    float cylinderTopRadius = 0.4f;
    float cylinderHeight = 0.75f;
    int cylinderSectorCount = 36;
    UCreateCylinder(cylinderMesh, cylinderBaseRadius, cylinderTopRadius, cylinderHeight, cylinderSectorCount);


    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    // Table texture
    const char* texFilename = "../../resources/textures/table.jpg";
    if (!UCreateTexture(texFilename, gTextureId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // Sphere texture
    const char* sphereTexFilename = "../../resources/textures/pink.jpg";
    if (!UCreateTexture(sphereTexFilename, sphereTextureId))
    {
        cout << "Failed to load texture " << sphereTexFilename << endl;
        return EXIT_FAILURE;
    }

    // Pumpkin texture
    const char* pumpkinTexFilename = "../../resources/textures/pumpkin.jpg"; // Adjust the path as needed
    if (!UCreateTexture(pumpkinTexFilename, pumpkinTextureId))
    {
        cout << "Failed to load texture " << pumpkinTexFilename << endl;
        return EXIT_FAILURE;
    }

    // Book texture
    const char* bookTexFilename = "../../resources/textures/blue.jpg";
    if (!UCreateTexture(bookTexFilename, bookTextureId))
    {
        cout << "Failed to load texture " << bookTexFilename << endl;
        return EXIT_FAILURE;
    }

    // Perfume (cylinder) texture
    const char* perfumeTexFilename = "../../resources/textures/Diamond.jpg";
    if (!UCreateTexture(perfumeTexFilename, perfumeTextureId))
    {
        cout << "Failed to load texture " << perfumeTexFilename << endl;
        return EXIT_FAILURE;
    }

    // Candle texture
    const char* cylinderTexFilename = "../../resources/textures/candle.png";
    if (!UCreateTexture(cylinderTexFilename, cylinderTextureId))
    {
        cout << "Failed to load texture " << cylinderTexFilename << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Background color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        UProcessInput(gWindow);

        URender();

        glfwPollEvents();
    }

    // All destroyed Mesh + Textures

    UDestroyMesh(gMesh);

    UDestroyMesh(bookMesh);

    UDestroyMesh(pumpkinMesh);

    UDestroyMesh(ellipticalCylinderMesh);

    UDestroyMesh(cylinderMesh);

    UDestroyTexture(gTextureId);

    UDestroyTexture(sphereTextureId);

    UDestroyTexture(pumpkinTextureId);

    UDestroyTexture(bookTextureId);

    UDestroyTexture(perfumeTextureId);

    UDestroyTexture(cylinderTextureId);


    UDestroyShaderProgram(gProgramId);

    exit(EXIT_SUCCESS);
}

// Initialize windows
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Creating window
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}

void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 3.0f; // Camera Speed

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) // Escape
        glfwSetWindowShouldClose(window, true);

    // ***** Input for W + A + S D *****
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);

    // ***** Input for Q + E keys *****
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);

    // If "P" key is pressed
    bool pKeyPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;

    // Activate different projection
    if (pKeyPressed && !pKeyWasPressed)
    {
        isPerspective = !isPerspective;
    }

    pKeyWasPressed = pKeyPressed;
}

void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// When mouse moves this is called
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos;

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// When mouse scroll, this is called
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// Mouse button events
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
        // Mouse left
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse pressed" << endl;
        else
            cout << "Left mouse released" << endl;
    }
    break;

    // Mouse middle
    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse pressed" << endl;
        else
            cout << "Middle mouse released" << endl;
    }
    break;

    // Mouse right
    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse pressed" << endl;
        else
            cout << "Right mouse released" << endl;
    }
    break;

    // Random button pressed
    default:
        cout << "Unhandled mouse event" << endl;
        break;
    }
}


// Created separate function for shader matrices
// Had to call model twice to position the sphere so I created a separate function
void shaderMatrices(glm::mat4 model, glm::mat4 view, glm::mat4 projection) {
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void URender()
{
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Camera view
    glm::mat4 view = gCamera.GetViewMatrix();

    // Perspective projection
    glm::mat4 projection;
    if (isPerspective)
    {
        // alternate projection
        projection = glm::perspective(glm::radians(gCamera.Zoom), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else
    {
        projection = glm::ortho(-2.0f, 2.0f, -1.5f, 1.5f, 0.1f, 100.0f); // Orthographic projection
    }


    // Light postion
    glm::vec3 lightPos = glm::vec3(2.0f, 2.0f, 3.0f); // Position

    // Ambient + diffuse light
    glm::vec3 ambientLightColor = glm::vec3(1.0f, 1.0f, 1.0f); // white
    glm::vec3 diffuseLightColor = glm::vec3(0.2f, 0.0f, 0.0f); // pinkish diffuse

    glUniform3fv(glGetUniformLocation(gProgramId, "ambientLightColor"), 1, glm::value_ptr(ambientLightColor));
    glUniform3fv(glGetUniformLocation(gProgramId, "diffuseLightColor"), 1, glm::value_ptr(diffuseLightColor));
    glUniform3fv(glGetUniformLocation(gProgramId, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(gProgramId, "viewPos"), 1, glm::value_ptr(gCamera.Position));

    /* The following code below is shown in order of:
        1. binding
        2. texture
        3. scale, translate, rotate, etc
        4. shader
        5. draw                                      
     */


    // Table
    glBindVertexArray(planeMesh.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.25f, 0.0f));
    shaderMatrices(model, view, projection);
    glDrawElements(GL_TRIANGLES, planeMesh.nVertices, GL_UNSIGNED_INT, 0);

    // Perfume top
    glBindVertexArray(sphereMesh.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sphereTextureId);
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.3f, 0.45f, 0.0f));
    shaderMatrices(model, view, projection);
    glDrawElements(GL_TRIANGLES, sphereMesh.nVertices, GL_UNSIGNED_INT, 0);

    // Book
    glBindVertexArray(bookMesh.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bookTextureId);
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.2f, -0.25f, 1.5f)); 
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(60.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(1.50f, 0.75f, 0.25f));
    shaderMatrices(model, view, projection);
    glDrawElements(GL_TRIANGLES, bookMesh.nVertices, GL_UNSIGNED_INT, 0);

    // Pumpkin 
    glBindVertexArray(pumpkinMesh.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pumpkinTextureId);
    model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.3f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    shaderMatrices(model, view, projection);
    glDrawElements(GL_TRIANGLES, pumpkinMesh.nVertices, GL_UNSIGNED_INT, 0);

    // Perfume base
    glBindVertexArray(ellipticalCylinderMesh.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, perfumeTextureId);
    model = glm::translate(model, glm::vec3(-1.3f, -0.3f, 0.0f));
    model = glm::scale(model, glm::vec3(1.75f, 0.8f, 1.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    shaderMatrices(model, view, projection);
    glDrawElements(GL_TRIANGLES, ellipticalCylinderMesh.nVertices, GL_UNSIGNED_INT, 0);

    // Candle
    glBindVertexArray(cylinderMesh.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cylinderTextureId);
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.0, 0.5f));
    shaderMatrices(model, view, projection);
    glDrawElements(GL_TRIANGLES, cylinderMesh.nVertices, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

    glfwSwapBuffers(gWindow);
}

// Sphere creation
void UCreateSphere(GLMesh& mesh, float r, int sectors, int stacks) {

    vector<float> vertices;
    vector<unsigned int> indices;

    float x, y, z, xy;
    float nx, ny, nz, lengthInv = 1.0f / r;
    float s, t;

    float sectorStep = 2 * 3.1415296f / sectors;
    float stackStep = 3.1415296f / stacks;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stacks; ++i) {
        stackAngle = 3.1415296f / 2 - i * stackStep;
        xy = r * cosf(stackAngle);
        z = r * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            sectorAngle = j * sectorStep;

            // Vertex (x, y, z)
            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Normal vertices (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);

            // Vertice coordinates
            s = (float)j / sectors;
            t = (float)i / stacks;
            vertices.push_back(s);
            vertices.push_back(t);
        }
    }

    int k1, k2;
    for (int i = 0; i < stacks; ++i) {
        k1 = i * (sectors + 1);
        k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {

            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mesh.nVertices = indices.size();
}


// Plane 
void UCreatePlane(GLMesh& mesh) {

    GLfloat vertices[] = {
        // Positions          // Colors         // Texture Coords
       -5.0f, -0.65f,  5.0f,  0.2f, 0.2f, 0.2f, 1.0f,  0.0f, 0.0f, // TL
        5.0f, -0.65f,  5.0f,  0.2f, 0.2f, 0.2f, 1.0f,  1.0f, 0.0f, // TR
        5.0f, -0.65f, -5.0f,  0.2f, 0.2f, 0.2f, 1.0f,  1.0f, 1.0f, // BR
       -5.0f, -0.65f, -5.0f,  0.2f, 0.2f, 0.2f, 1.0f,  0.0f, 1.0f, // BL
    };

    // Two triangles that share 2 indices
    GLuint indices[] = {
        0, 1, 2, // First triangle
        0, 2, 3  // Second triangle
    };

    mesh.nVertices = 6; // Two triangles, three vertices each

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Texture
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*)(7 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void UCreateBook(GLMesh& mesh) {

    GLfloat bookVertices[] = {

        // Front face
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,

        // Back face 
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,

        // Left face
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 

        // Right face
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 

    };

    GLuint bookIndices[] = {
        0, 1, 2, 0, 2, 3,   
        4, 5, 6, 4, 6, 7,   
        0, 4, 7, 0, 7, 3,   
        1, 5, 6, 1, 6, 2,   
        0, 1, 5, 0, 5, 4,   
        3, 2, 6, 3, 6, 7    
    };

    int numVertices = sizeof(bookVertices) / sizeof(bookVertices[0]);
    int numIndices = sizeof(bookIndices) / sizeof(bookIndices[0]);

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    GLuint vbo, ebo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(GLfloat), bookVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLuint), bookIndices, GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mesh.nVertices = numIndices;
}

void UCreateEllipticalCylinder(GLMesh& mesh, float baseRadius, float topRadius, float height, int sectorCount) {
    vector<GLfloat> vertices;
    vector<GLuint> indices;

    float angleStep = 2 * 3.1415296f / sectorCount;
    float halfHeight = height / 2.0f;

    // Top + bottom cylinder
    for (int i = 0; i <= sectorCount; ++i) {
        float angle = i * angleStep;
        float x = baseRadius * cos(angle);
        float z = baseRadius * sin(angle);
        float u = (float)i / sectorCount;

        // Top circle
        vertices.push_back(x);
        vertices.push_back(halfHeight);
        vertices.push_back(z);

        // Normal up
        vertices.push_back(0);
        vertices.push_back(-1);
        vertices.push_back(0);
        vertices.push_back(u);
        vertices.push_back(0.0f);

        // Bottom circle
        vertices.push_back(x);
        vertices.push_back(-halfHeight);
        vertices.push_back(z);

        // Normal down
        vertices.push_back(0);
        vertices.push_back(-1);
        vertices.push_back(0);
        vertices.push_back(u);
        vertices.push_back(1.0f);
    }

    // Top + bottom cap and sides
    for (int i = 0; i < sectorCount; ++i) {
        int topIndex1 = i * 2;
        int topIndex2 = (i + 1) * 2 % (sectorCount * 2);
        int bottomIndex1 = i * 2 + 1;
        int bottomIndex2 = (topIndex2 + 1) % (sectorCount * 2);

        // Top cap
        indices.push_back(topIndex1);
        indices.push_back(topIndex2);
        indices.push_back(sectorCount * 2);

        // Bottom cap
        indices.push_back(bottomIndex2);
        indices.push_back(bottomIndex1);
        indices.push_back(sectorCount * 2 + 1);

        // Sides
        indices.push_back(topIndex1);
        indices.push_back(bottomIndex1);
        indices.push_back(bottomIndex2);

        indices.push_back(topIndex1);
        indices.push_back(bottomIndex2);
        indices.push_back(topIndex2);
    }

    int topCenterIndex = vertices.size() / 6;
    vertices.push_back(0);
    vertices.push_back(halfHeight);
    vertices.push_back(0);
    vertices.push_back(0);
    vertices.push_back(1);
    vertices.push_back(0);
    vertices.push_back(0.5f);
    vertices.push_back(0.5f);

    int bottomCenterIndex = vertices.size() / 6;
    vertices.push_back(0);
    vertices.push_back(-halfHeight);
    vertices.push_back(0);
    vertices.push_back(0);
    vertices.push_back(-1);
    vertices.push_back(0);
    vertices.push_back(0.5f);
    vertices.push_back(0.5f);

    // Top + bottom caps
    for (int i = 0; i < sectorCount; ++i) {
        int topIndex1 = i * 2;
        int topIndex2 = (i + 1) * 2;
        int bottomIndex1 = i * 2 + 1;
        int bottomIndex2 = (i + 1) * 2 + 1;

        // Top cap
        indices.push_back(topCenterIndex);
        indices.push_back(topIndex1);
        indices.push_back(topIndex2);

        // Bottom cap
        indices.push_back(bottomIndex1);
        indices.push_back(bottomIndex2);
        indices.push_back(bottomCenterIndex);
    }

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Vertex
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Texture
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mesh.nVertices = indices.size();
}

void UCreateCylinder(GLMesh& mesh, float baseRadius, float topRadius, float height, int sectorCount) {
    vector<GLfloat> vertices;
    vector<GLuint> indices;

    float angleStep = 2 * 3.1415296f / sectorCount;
    float halfHeight = height / 2.0f;

    // Vetices for top + bottom circles
    for (int i = 0; i <= sectorCount; ++i) {
        float angle = i * angleStep;
        float x = cos(angle);
        float z = sin(angle);

        // Top circle 

        vertices.push_back(x * topRadius);
        vertices.push_back(halfHeight);
        vertices.push_back(z * topRadius);

        // Normal up
        vertices.push_back(0);
        vertices.push_back(1);
        vertices.push_back(0);

        // Bottom circle 
        vertices.push_back(x * baseRadius);
        vertices.push_back(-halfHeight);
        vertices.push_back(z * baseRadius);

        // Normal down
        vertices.push_back(0);
        vertices.push_back(1);
        vertices.push_back(0);
    }

    // Indices top bottom + sides
    for (int i = 0; i < sectorCount; ++i) {
        int topIndex1 = i * 2;
        int topIndex2 = (i + 1) * 2 % (sectorCount * 2);
        int bottomIndex1 = i * 2 + 1;
        int bottomIndex2 = (topIndex2 + 1) % (sectorCount * 2);

        // Top
        indices.push_back(topIndex1);
        indices.push_back(topIndex2);
        indices.push_back(sectorCount * 2);

        // Bottom 
        indices.push_back(bottomIndex1);
        indices.push_back(bottomIndex2);
        indices.push_back(sectorCount * 2 + 1);

        // Side 
        indices.push_back(topIndex1);
        indices.push_back(bottomIndex1);
        indices.push_back(topIndex2);

        indices.push_back(bottomIndex1);
        indices.push_back(bottomIndex2);
        indices.push_back(topIndex2);
    }

    vertices.push_back(0);
    vertices.push_back(halfHeight);
    vertices.push_back(0);
    vertices.push_back(0);
    vertices.push_back(1);
    vertices.push_back(0);

    vertices.push_back(0);
    vertices.push_back(-halfHeight);
    vertices.push_back(0);
    vertices.push_back(0);
    vertices.push_back(-1);
    vertices.push_back(0);


    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Vertext
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    mesh.nVertices = indices.size();
}

void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}

bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

void UDestroyTexture(GLuint textureId)
{
    glDeleteTextures(1, &textureId);
}

bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}

void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}