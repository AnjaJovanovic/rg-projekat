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
unsigned int loadTexture(char const *path);

bool rainy = false;
bool lampOn = true;

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

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

    glm::vec3 airplanePosition = glm::vec3(200.0f, 150.0f, 30.0f);
    float airplaneScale = 3.5f;

    glm::vec3 lampPosition = glm::vec3(-10.0f, -80.0f, -10.0f);
    float lampScale = 10.0f;

    PointLight pointLight;
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

    // load models
    // -----------

    // airplane
    Model airplane("resources/objects/airplane/piper_pa18.obj");
    airplane.SetShaderTextureNamePrefix("material.");

    // island
    Model island("resources/objects/SmallTropicalIsland/Small_Tropical_Island.obj");
    island.SetShaderTextureNamePrefix("material.");

    // lamp
    Model lamp("resources/objects/Street_lamp_7_OBJ/Street_Lamp_7.obj");
    lamp.SetShaderTextureNamePrefix("material.");

    DirLight& dirLight = programState->dirLight;

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(-10.8f, -59.0, -9.56);
    pointLight.ambient = glm::vec3(10.0, 10.0, 10.0);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 14.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

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

    // rain vertices
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
    unsigned int rainTexture = loadTexture(FileSystem::getPath("resources/textures/rain.png").c_str());

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
    unsigned int cubemapTextureRainy = loadCubemap(facesRainy);
    unsigned int cubemapTextureSunny = loadCubemap(facesSunny);

    // shader configuration
    // --------------------
    skyboxShader.use();
    skyboxShader.setInt("skybox",0);

    blendingShader.use();
    blendingShader.setInt("texture1", 0);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
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
        if (lampOn) {
            pointLight.ambient = glm::vec3(10.0, 10.0, 10.0);
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
        ourShader.setFloat("material.shininess", 32.0f);

        // Directional light
        if(rainy) {
            dirLight.direction = glm::vec3(0.0f, -30.0f, -42.0f);
            dirLight.ambient = glm::vec3(0.25f);
            dirLight.diffuse = glm::vec3( 0.25f);
            dirLight.specular = glm::vec3(0.25f);
        } else {
            dirLight.direction = glm::vec3(0.0f, -30.0f, -42.0f);
            dirLight.ambient = glm::vec3(0.3f);
            dirLight.diffuse = glm::vec3( 0.3f);
            dirLight.specular = glm::vec3(0.3f);
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
        glm::vec3 rotationCenter = glm::vec3(30, 10, 10);
        glm::mat4 translateToOrigin = glm::translate(glm::mat4(1.0f), -rotationCenter);
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), (float)currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 translateBack = glm::translate(glm::mat4(1.0f), rotationCenter);
        airplaneModel = translateBack * rotationMatrix * translateToOrigin;
        airplaneModel = glm::scale(airplaneModel, glm::vec3(programState->airplaneScale));
        ourShader.setMat4("model", airplaneModel);
        airplane.Draw(ourShader);

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

        // rain
        blendingShader.use();
        glm::mat4 rainM = glm::mat4(1.0f);
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);
        glBindVertexArray(transparentVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rainTexture);
        float rainSpeed = 0.1f; // Brzina pada kiše
        if(rainy) {
            for (unsigned int i = 0; i < 30000; i++) {
                // Ažuriranje pozicije kišnih kapljica
                rainPositions[i].y -= rainSpeed;

                // Provera da li je kapljica van granica ekrana
                if (rainPositions[i].y < -50.0f) {
                    // Ako je van granica, postavite je na vrh ekrana
                    rainPositions[i].y = 50.0f;
                }

                rainM = glm::mat4(1.0f);
                rainM = glm::scale(rainM, glm::vec3(1.5f));
                rainM = glm::translate(rainM, rainPositions[i]);
                rainM = glm::rotate(rainM ,glm::radians(rainRotation[i]), glm::vec3(0.0f ,1.0f, 0.0f));
                blendingShader.setMat4("model", rainM);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        glEnable(GL_CULL_FACE);

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
        } else {
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureSunny);
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
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
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
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if(key == GLFW_KEY_R && action == GLFW_PRESS) {
        rainy = !rainy;
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        lampOn = !lampOn;
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

unsigned int loadTexture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data){
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else{
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}