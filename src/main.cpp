#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);
unsigned int loadTexture(const char *path, bool gammaCorrection);
void renderQuad();

// weather settings
bool rainy = false;
bool sunny = true;
bool storm = false;

bool lampOn = true;
bool houseLampOn = false;

// airplane settings
bool planeCrash = false;
glm::vec3 currentAirplanePosition;
glm::mat4 currentAirplaneRotation = glm::mat4(1.0f);
float airplaneHeight = -95.0;

//lightning settings
int randomLightningSpawn = 30;
int lightningFrameDuration = 0;

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
float heightScale = 0.1;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;

    glm::vec3 islandPosition = glm::vec3(0.0f, -100.0f, 0.0f);
    float islandScale = 0.7f;

    glm::vec3 airplanePosition = glm::vec3(200.0f, 250.0f, 30.0f);
    float airplaneScale = 3.5f;

    glm::vec3 boatPosition = glm::vec3(80.0f, -97.0f, 120.0f);
    float boatScale = 1.0f;

    glm::vec3 lampPosition = glm::vec3(-10.0f, -80.0f, -10.0f);
    float lampScale = 10.0f;

    glm::vec3 tablePosition = glm::vec3(-5.0f, -77.0f, 11.0f);
    float tableScale = 4.0f;

    glm::vec3 chairPosition = glm::vec3(-5.0f, -77.0f, 8.0f);
    float chairScale = 4.0f;

    glm::vec3 houseLampPosition = glm::vec3(-13.0f, -71.0f, 9.0f);
    float houseLampScale = 4.0f;

    glm::vec3 applePosition = glm::vec3(-5.0f, -74.0f, 11.0f);
    float appleScale = 0.05f;


    PointLight pointLight;
    PointLight pointLightHouse;
    DirLight dirLight;

    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 0.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    glfwWindowHint(GLFW_SAMPLES, 4);

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_MULTISAMPLE);

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // Face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/model.vs", "resources/shaders/model.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader blendingShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    Shader parallaxShader("resources/shaders/parallax_mapping.vs", "resources/shaders/parallax_mapping.fs");

    // load models
    // -----------

    // airplane
    Model airplane("resources/objects/airplane/piper_pa18.obj");
    airplane.SetShaderTextureNamePrefix("material.");

    // boat
    Model boat("resources/objects/OldBoat/OldBoat.obj");
    boat.SetShaderTextureNamePrefix("material.");

    // island
    Model island("resources/objects/SmallTropicalIsland/Small_Tropical_Island.obj");
    island.SetShaderTextureNamePrefix("material.");

    // lamp
    Model lamp("resources/objects/Street_lamp_7_OBJ/Street_Lamp_7.obj");
    lamp.SetShaderTextureNamePrefix("material.");

    // table
    Model table("resources/objects/WoodenTable/Table.obj");
    table.SetShaderTextureNamePrefix("material.");

    // chair
    Model chair("resources/objects/WoodenTable/Chair.obj");
    chair.SetShaderTextureNamePrefix("material.");

    // house lamp
    Model houselamp("resources/objects/light/Light.obj");
    houselamp.SetShaderTextureNamePrefix("material.");

    // apple
    Model apple("resources/objects/apple/apple.obj");
    apple.SetShaderTextureNamePrefix("material.");

    // Directional light
    // -----------------
    DirLight& dirLight = programState->dirLight;

    // Point light
    // -----------
    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(-10.8f, -59.0f, -9.56f);
    pointLight.ambient = glm::vec3(10.0, 10.0, 10.0);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 14.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    PointLight& pointLightHouse = programState->pointLightHouse;
    pointLightHouse.position = glm::vec3(-11.8f, -71.0f, 8.0f);
    pointLightHouse.ambient = glm::vec3(10.0, 10.0, 10.0);
    pointLightHouse.diffuse = glm::vec3(10.3, 10.3, 10.3);
    pointLightHouse.specular = glm::vec3(5.1, 5.1, 5.1);

    pointLightHouse.constant = 19.0f;
    pointLightHouse.linear = 0.09f;
    pointLightHouse.quadratic = 0.032f;

    // Skybox vertices
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // transparent vertices
    float transparentVertices[] = {
            // positions         // texture Coords
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f,

            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f
    };

    // rain positions
    vector<glm::vec3> rainPositions;
    vector<float> rainRotation;

    for (int i = 0; i < 30000; i++) {
        float x = static_cast<float>(rand() % 201 - 100); // x koordinate u rasponu [-100, 100]
        float y = static_cast<float>(rand() % 401 - 200); // y koordinate u rasponu [-200, 200]
        float z = static_cast<float>(rand() % 201 - 100); // z koordinate u rasponu [-100, 100]

        rainPositions.push_back(glm::vec3(x, y, z));
        rainRotation.push_back(static_cast<float>(i % 180)); // Rotacija u rasponu [0, 179]
    }

    // skybox VAO VBO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // transparent VAO VBO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // load textures
    // -------------

    // rain texture
    unsigned int rainTexture = loadTexture(FileSystem::getPath("resources/textures/rain.png").c_str(), true);

    // lightning texture)
    unsigned int lightningTexture = loadTexture(FileSystem::getPath("resources/textures/lighting.png").c_str(), true);

    // floor texture
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/floor/wood_0041_color_2k.jpg").c_str(), true);
    unsigned int normalMap  = loadTexture(FileSystem::getPath("resources/textures/floor/wood_0041_normal_opengl_2k.png").c_str(), true);
    unsigned int heightMap  = loadTexture(FileSystem::getPath("resources/textures/floor/wood_0041_height_2k.png").c_str(), true);
    
    // skybox textures
    stbi_set_flip_vertically_on_load(false);
    vector<std::string> facesSunny = {

            FileSystem::getPath("resources/textures/skyboxSun/px.jpg"),
            FileSystem::getPath("resources/textures/skyboxSun/nx.jpg"),
            FileSystem::getPath("resources/textures/skyboxSun/py.jpg"),
            FileSystem::getPath("resources/textures/skyboxSun/ny.jpg"),
            FileSystem::getPath("resources/textures/skyboxSun/pz.jpg"),
            FileSystem::getPath("resources/textures/skyboxSun/nz.jpg")
    };
    vector<std::string> facesRainy = {

            FileSystem::getPath("resources/textures/skyboxRain/px.jpg"),
            FileSystem::getPath("resources/textures/skyboxRain/nx.jpg"),
            FileSystem::getPath("resources/textures/skyboxRain/py.jpg"),
            FileSystem::getPath("resources/textures/skyboxRain/ny.jpg"),
            FileSystem::getPath("resources/textures/skyboxRain/pz.jpg"),
            FileSystem::getPath("resources/textures/skyboxRain/nz.jpg")

    };
    vector<std::string> facesStorm = {

            FileSystem::getPath("resources/textures/skyboxStorm/px.jpg"),
            FileSystem::getPath("resources/textures/skyboxStorm/nx.jpg"),
            FileSystem::getPath("resources/textures/skyboxStorm/py.jpg"),
            FileSystem::getPath("resources/textures/skyboxStorm/ny.jpg"),
            FileSystem::getPath("resources/textures/skyboxStorm/pz.jpg"),
            FileSystem::getPath("resources/textures/skyboxStorm/nz.jpg")

    };
    unsigned int cubemapTextureRainy = loadCubemap(facesRainy);
    unsigned int cubemapTextureSunny = loadCubemap(facesSunny);
    unsigned int cubemapTextureStorm = loadCubemap(facesStorm);

    // shader configuration
    // --------------------
    skyboxShader.use();
    skyboxShader.setInt("skybox",0);

    blendingShader.use();
    blendingShader.setInt("texture1", 0);

    parallaxShader.use();
    parallaxShader.setInt("diffuseMap", 0);
    parallaxShader.setInt("normalMap", 1);
    parallaxShader.setInt("depthMap", 2);

    //draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        //randomLightningSpawn = rand()%15;
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        // Point light
        // -----------
        //outdoor
        if (lampOn) {
            pointLight.ambient = glm::vec3(8.0, 8.0, 8.0);
        } else {
            pointLight.ambient = glm::vec3(0.0, 0.0, 0.0);
        }

        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setVec3("lightPos", pointLight.position);
        ourShader.setFloat("material.shininess", 32.0f);

        // indoor
        if (houseLampOn) {
            pointLightHouse.ambient = glm::vec3(2.5f, 2.5f, 0.0f);
        } else {
            pointLightHouse.ambient = glm::vec3(0.0, 0.0, 0.0);
        }

        ourShader.setVec3("pointLightHouse.position", pointLightHouse.position);
        ourShader.setVec3("pointLightHouse.ambient", pointLightHouse.ambient);
        ourShader.setVec3("pointLightHouse.diffuse", pointLightHouse.diffuse);
        ourShader.setVec3("pointLightHouse.specular", pointLightHouse.specular);
        ourShader.setFloat("pointLightHouse.constant", pointLightHouse.constant);
        ourShader.setFloat("pointLightHouse.linear", pointLightHouse.linear);
        ourShader.setFloat("pointLightHouse.quadratic", pointLightHouse.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setVec3("lightPos", pointLightHouse.position);
        ourShader.setFloat("material.shininess", 32.0f);


        // Directional light
        if(rainy) {
            dirLight.direction = glm::vec3(0.0f, -30.0f, -42.0f);
            dirLight.ambient = glm::vec3(0.25f);
            dirLight.diffuse = glm::vec3( 0.25f);
            dirLight.specular = glm::vec3(0.25f);
        } else if(sunny) {
            dirLight.direction = glm::vec3(0.0f, -30.0f, -42.0f);
            dirLight.ambient = glm::vec3(0.3f);
            dirLight.diffuse = glm::vec3( 0.3f);
            dirLight.specular = glm::vec3(0.3f);
        } else {
            if(storm && lightningFrameDuration == 0) {
                dirLight.ambient = glm::vec3(0.1f);
            } else {
                dirLight.ambient = glm::vec3(0.0f);
            }
            dirLight.direction = glm::vec3(0.0f, -30.0f, -42.0f);
            dirLight.diffuse = glm::vec3( 0.2f);
            dirLight.specular = glm::vec3(0.2f);
        }
        ourShader.setVec3("dirLight.direction", dirLight.direction);
        ourShader.setVec3("dirLight.ambient", dirLight.ambient);
        ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        ourShader.setVec3("dirLight.specular", dirLight.specular);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render the loaded model
        //------------------------

        // airplane
        glm::mat4 airplaneModel = glm::mat4(1.0f);
        airplaneModel = glm::translate(airplaneModel, programState->airplanePosition);

        if (planeCrash == false) {
            glm::vec3 rotationCenter = glm::vec3(30, 10, 10);
            glm::mat4 translateToOrigin = glm::translate(glm::mat4(1.0f), -rotationCenter);
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), (float) currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 translateBack = glm::translate(glm::mat4(1.0f), rotationCenter);
            airplaneModel = translateBack * rotationMatrix * translateToOrigin;
            airplaneModel = glm::scale(airplaneModel, glm::vec3(programState->airplaneScale));
            currentAirplanePosition = glm::vec3(airplaneModel[3]);
            currentAirplaneRotation = rotationMatrix;
        } else {
            // Airplane falling down
            airplaneModel = glm::scale(airplaneModel, glm::vec3(programState->airplaneScale));
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            airplaneModel *= rotationMatrix;
            airplaneModel *= currentAirplaneRotation;
            programState->airplanePosition = currentAirplanePosition;

            if (currentAirplanePosition.y > airplaneHeight) {
                if (currentAirplaneRotation[0][0] > 0 && currentAirplaneRotation[1][1] > 0) {
                    // Prvi kvadrant
                } else if (currentAirplaneRotation[0][0] < 0 && currentAirplaneRotation[1][1] > 0) {
                    // Drugi kvadrant
                    glm::mat4 correctionRotation = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                    airplaneModel *= correctionRotation;
                } else if (currentAirplaneRotation[0][0] < 0 && currentAirplaneRotation[1][1] < 0) {
                    // Treći kvadrant
                    glm::mat4 correctionRotation = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                    airplaneModel *= correctionRotation;
                } else if (currentAirplaneRotation[0][0] > 0 && currentAirplaneRotation[1][1] < 0) {
                    // Četvrti kvadrant
                }
                currentAirplanePosition += glm::vec3(0.4f, -0.6f, 0.06f);
            }
        }
        ourShader.setMat4("model", airplaneModel);
        airplane.Draw(ourShader);

        // boat
        glm::mat4 boatModel = glm::mat4(1.0f);
        boatModel = glm::translate(boatModel,
                                   programState->boatPosition); // translate it down so it's at the center of the scene
        boatModel = glm::scale(boatModel, glm::vec3(programState->boatScale));    // it's a bit too big for our scene, so scale it down
        boatModel = glm::rotate(boatModel, glm::radians(-60.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", boatModel);
        boat.Draw(ourShader);

        // island
        glm::mat4 islandModel = glm::mat4(1.0f);
        islandModel = glm::translate(islandModel,
                               programState->islandPosition); // translate it down so it's at the center of the scene
        islandModel = glm::scale(islandModel, glm::vec3(programState->islandScale));    // it's a bit too big for our scene, so scale it down
        islandModel = glm::rotate(islandModel, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", islandModel);
        island.Draw(ourShader);

        // lamp
        glm::mat4 lampModel = glm::mat4(1.0f);
        lampModel = glm::translate(lampModel,
                                     programState->lampPosition); // translate it down so it's at the center of the scene
        lampModel = glm::scale(lampModel, glm::vec3(programState->lampScale));    // it's a bit too big for our scene, so scale it down
        lampModel = glm::rotate(lampModel, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", lampModel);
        lamp.Draw(ourShader);

        // table
        glm::mat4 tableModel = glm::mat4(1.0f);
        tableModel = glm::translate(tableModel,
                                   programState->tablePosition); // translate it down so it's at the center of the scene
        tableModel = glm::scale(tableModel, glm::vec3(programState->tableScale));    // it's a bit too big for our scene, so scale it down
        tableModel = glm::rotate(tableModel, glm::radians(74.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", tableModel);
        table.Draw(ourShader);

        // chairs
        glm::mat4 chairModel1 = glm::mat4(1.0f);
        chairModel1 = glm::translate(chairModel1,
                                    programState->chairPosition); // translate it down so it's at the center of the scene
        chairModel1 = glm::scale(chairModel1, glm::vec3(programState->chairScale));    // it's a bit too big for our scene, so scale it down
        chairModel1 = glm::rotate(chairModel1, glm::radians(74.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", chairModel1);
        chair.Draw(ourShader);

        glm::mat4 chairModel2 = glm::mat4(1.0f);
        chairModel2 = glm::translate(chairModel2,
                                     programState->chairPosition + glm::vec3(0.0f,0.0f,6.0f)); // translate it down so it's at the center of the scene
        chairModel2 = glm::scale(chairModel2, glm::vec3(programState->chairScale));    // it's a bit too big for our scene, so scale it down
        chairModel2 = glm::rotate(chairModel2, glm::radians(74.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", chairModel2);
        chair.Draw(ourShader);

        // house lamp
        glm::mat4 houseLampModel = glm::mat4(1.0f);
        houseLampModel = glm::translate(houseLampModel,
                                     programState->houseLampPosition); // translate it down so it's at the center of the scene
        houseLampModel = glm::scale(houseLampModel, glm::vec3(programState->houseLampScale));    // it's a bit too big for our scene, so scale it down
        houseLampModel = glm::rotate(houseLampModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        houseLampModel = glm::rotate(houseLampModel, glm::radians(-76.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ourShader.setMat4("model", houseLampModel);
        houselamp.Draw(ourShader);

        //apple
        glm::mat4 appleModel = glm::mat4(1.0f);
        appleModel = glm::translate(appleModel,
                                        programState->applePosition); // translate it down so it's at the center of the scene
        appleModel = glm::scale(appleModel, glm::vec3(programState->appleScale));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", appleModel);
        apple.Draw(ourShader);


        // Front
        /*glm::mat4 quad = glm::mat4(1.0f);
        quad = glm::translate(quad, glm::vec3(-35.0f, -85.0f, 25.0f));
        quad = glm::scale(quad, glm::vec3(4.0f));
        parallaxShader.setMat4("model", quad);
        renderQuad();

        // Back
        quad = glm::mat4(1.0f);
        quad = glm::translate(quad, glm::vec3(-35.0f, -85.0f, 17.0f));
        quad = glm::rotate(quad, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        quad = glm::scale(quad, glm::vec3(4.0f));
        parallaxShader.setMat4("model", quad);
        renderQuad();

        // Left
        quad = glm::mat4(1.0f);
        quad = glm::translate(quad, glm::vec3(-31.0f, -85.0f, 21.0f));
        quad = glm::rotate(quad, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        quad = glm::scale(quad, glm::vec3(4.0f));
        parallaxShader.setMat4("model", quad);
        renderQuad();

        // Right
        quad = glm::mat4(1.0f);
        quad = glm::translate(quad, glm::vec3(-39.0f, -85.0f, 21.0f));
        quad = glm::rotate(quad, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        quad = glm::scale(quad, glm::vec3(4.0f));
        parallaxShader.setMat4("model", quad);
        renderQuad();

        // Top
        quad = glm::mat4(1.0f);
        quad = glm::translate(quad, glm::vec3(-35.0f, -81.0f, 21.0f));
        quad = glm::rotate(quad, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        quad = glm::scale(quad, glm::vec3(4.0f));
        parallaxShader.setMat4("model", quad);
        renderQuad();*/

        // House floor
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, heightMap);

        parallaxShader.use();

        parallaxShader.setMat4("projection", projection);
        parallaxShader.setMat4("view", view);
        parallaxShader.setVec3("viewPos", programState->camera.Position);
        parallaxShader.setVec3("lightPos", pointLight.position);
        parallaxShader.setFloat("heightScale", heightScale);

        glm::mat4 quad = glm::mat4(1.0f);
        quad = glm::translate(quad, glm::vec3(-5.0f, -76.5f, 11.5f));
        quad = glm::rotate(quad, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        quad = glm::rotate(quad, glm::radians(-17.5f), glm::vec3(0.0f, 0.0f, 1.0f));
        quad = glm::scale(quad, glm::vec3(11.0f, 6.7f, 7.5f));
        parallaxShader.setMat4("model", quad);
        renderQuad();


        // rain
        blendingShader.use();
        glm::mat4 rainM = glm::mat4(1.0f);
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);
        glBindVertexArray(transparentVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rainTexture);
        float rainSpeed = 0.1f; // Brzina pada kiše
        if(rainy || storm) {
            for (unsigned int i = 0; i < 30000; i++) {
                // Ažuriranje pozicije kišnih kapljica
                rainPositions[i].y -= rainSpeed;

                // Provera da li je kapljica van granica ekrana
                if (rainPositions[i].y < -50.0f) {
                    // Ako je van granica, postavite je na vrh ekrana
                    rainPositions[i].y = 50.0f;
                }

                rainM = glm::mat4(1.0f);
                if(rainy)
                    rainM = glm::scale(rainM, glm::vec3(1.5f));
                else
                    rainM = glm::scale(rainM, glm::vec3(2.0f));
                
                rainM = glm::translate(rainM, rainPositions[i]);
                rainM = glm::rotate(rainM ,glm::radians(rainRotation[i]), glm::vec3(0.0f ,1.0f, 0.0f));
                blendingShader.setMat4("model", rainM);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        glEnable(GL_CULL_FACE);

        // lightning
        blendingShader.use();
        glm::mat4 lightningM = glm::mat4(1.0f);
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);
        glBindVertexArray(transparentVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, lightningTexture);
        if(storm && lightningFrameDuration == 0) {
            lightningM = glm::mat4(1.0f);
            lightningM = glm::scale(lightningM, glm::vec3(200.0f, rand() % 200 + 600.0f, 200.0f));
            float x = rand() % 6;
            float z = rand() % 6;
            lightningM = glm::translate(lightningM, glm::vec3(x-2.5 ,0.3f, z-2.5));
            lightningM = glm::rotate(lightningM,glm::radians(90.0f), glm::vec3(0.0f ,1.0f, 0.0f));
            blendingShader.setMat4("model", lightningM);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glEnable(GL_CULL_FACE);

        randomLightningSpawn = 60 + rand() % 50;
        lightningFrameDuration++;
        if(lightningFrameDuration > randomLightningSpawn)
            lightningFrameDuration = 0;

        // draw skybox
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);

        if(rainy) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureRainy);
        } else if (sunny){
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureSunny);
        } else {
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureStorm);
        }

        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &skyboxVBO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, 0.20);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, 0.20);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, 0.20);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, 0.20);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(DOWN, 0.20);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(UP, 0.20);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Airplane position", (float*)&programState->airplanePosition);
        ImGui::DragFloat("Airplane scale", &programState->airplaneScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::Bullet();
        ImGui::Text("C - Crush plane");
        ImGui::Bullet();
        ImGui::Text("P - ON/OFF pointLight");
        ImGui::Bullet();
        ImGui::Text("R - Change weather");
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::DragFloat("Movement Speed", (float *) &programState->camera.MovementSpeed, 0.5f, 0.5f, 50.0f);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            programState->CameraMouseMovementUpdateEnabled = true;
        }
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS && programState->ImGuiEnabled) {
        programState->CameraMouseMovementUpdateEnabled = !programState->CameraMouseMovementUpdateEnabled;
        if(programState->CameraMouseMovementUpdateEnabled == true)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        if (rainy) {
            rainy = false;
            storm = true;
        } else if (sunny) {
            sunny = false;
            rainy = true;
        } else {
            storm = false;
            sunny = true;
        }
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        lampOn = !lampOn;
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        houseLampOn = !houseLampOn;
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS){
        planeCrash = !planeCrash;
    }

    if (key == GLFW_KEY_Q && action == GLFW_PRESS){
        if (heightScale > 0.0f)
            heightScale -= 0.005f;
        else
            heightScale = 0.0f;
    }

    if (key == GLFW_KEY_E && action == GLFW_PRESS){
        if (heightScale < 1.0f)
            heightScale += 0.005f;
        else
            heightScale = 1.0f;
    }
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int loadTexture(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// renders a 1x1 quad in NDC with manually calculated tangent vectors
// ------------------------------------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        // positions
        glm::vec3 pos1(-1.0f,  1.0f, 0.0f);
        glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
        glm::vec3 pos3( 1.0f, -1.0f, 0.0f);
        glm::vec3 pos4( 1.0f,  1.0f, 0.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent1 = glm::normalize(tangent1);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent1 = glm::normalize(bitangent1);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent2 = glm::normalize(tangent2);

        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent2 = glm::normalize(bitangent2);

        float quadVertices[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}