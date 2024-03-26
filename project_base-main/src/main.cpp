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

unsigned int loadTexture(const char *path);

unsigned int loadCubemap(vector<std::string> faces);

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
bool blinn = false;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

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

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Parkic", NULL, NULL);
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

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

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

    //blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //face
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CW);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader shader("resources/shaders/cubemaps.vs", "resources/shaders/cubemaps.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader blendingShader("resources/shaders/2.model_lighting.vs", "resources/shaders/blending.fs" );
    Shader BlinnPhongshader("resources/shaders/1.advanced_lighting.vs", "resources/shaders/1.advanced_lighting.fs");
    // load models
    // -----------

    Model ourModel("resources/objects/klupa3/uploads_files_793049_Bank-of-Wooden;OBJ;FBX/Bank-of-Wooden;OBJ;FBX/Bank_of_Wooden.obj");
    ourModel.SetShaderTextureNamePrefix("material.");

    Model benchModel("resources/objects/klupa3/uploads_files_793049_Bank-of-Wooden;OBJ;FBX/Bank-of-Wooden;OBJ;FBX/Bank_of_Wooden.obj");
    benchModel.SetShaderTextureNamePrefix("material.");

    Model sunModel("resources/objects/sunce/Sun.obj");
    sunModel.SetShaderTextureNamePrefix("material.");

    stbi_set_flip_vertically_on_load(false);
    Model treeModel("resources/objects/tree5/uploads_files_2418161_ItalianCypress/ItalianCypress.obj");
    treeModel.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    stbi_set_flip_vertically_on_load(false);
    Model campFireModel("resources/objects/campfire6/fire/source/fireclean/fireclean.obj");
    campFireModel.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    stbi_set_flip_vertically_on_load(false);
    Model grassModel("resources/objects/grass2/Grass_free_obj/free grass by adam127.obj");
    grassModel.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);


    float planeVertices[] = {
            // positions            // normals         // texcoords
            20.0f, -0.5f, -20.0f,  0.0f, 1.0f, 0.0f,  20.0f, 20.0f, // Vertex 6
            -20.0f, -0.5f, -20.0f,  0.0f, 1.0f, 0.0f,   0.0f, 20.0f, // Vertex 5
            20.0f, -0.5f,  20.0f,  0.0f, 1.0f, 0.0f,  20.0f,  0.0f, // Vertex 4

            -20.0f, -0.5f, -20.0f,  0.0f, 1.0f, 0.0f,   0.0f, 20.0f, // Vertex 3
            -20.0f, -0.5f,  20.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f, // Vertex 2
            20.0f, -0.5f,  20.0f,  0.0f, 1.0f, 0.0f,  20.0f,  0.0f  // Vertex 1
    };
    float angleInDegrees = 90.0f;


    float angleInRadians = glm::radians(angleInDegrees);

// Kreiranje rotacione matrice oko Y ose za dati ugao
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angleInRadians, glm::vec3(0.0f, 1.0f, 0.0f));

// Rotiranje svaki verteks plana pomoću rotacione matrice
    for (int i = 0; i < sizeof(planeVertices) / sizeof(float); i += 8) {
        glm::vec4 vertex(planeVertices[i], planeVertices[i + 1], planeVertices[i + 2], 1.0f);
        vertex = rotationMatrix * vertex;
        planeVertices[i] = vertex.x;
        planeVertices[i + 1] = vertex.y;
        planeVertices[i + 2] = vertex.z;
    }

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

    //ovaj kod za travu sam nasao u opengl repozitorijumu 4.advanced_opengl/3.1.blending_discard
    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(1, 1, 1);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.0f;
    pointLight.quadratic = 0.0f;

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    //transparen VAO
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

    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    //load textures
    unsigned int GrassTexture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str());
    unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/textures/classic-green-grass-seamless-texture-free-photo.png").c_str());

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/Park/posx.jpg"),
                    FileSystem::getPath("resources/textures/skybox/Park/negx.jpg"),
                    FileSystem::getPath("resources/textures/skybox/Park/negy.jpg"),
                    FileSystem::getPath("resources/textures/skybox/Park/posy.jpg"),
                    FileSystem::getPath("resources/textures/skybox/Park/posz.jpg"),
                    FileSystem::getPath("resources/textures/skybox/Park/negz.jpg")
            };
    unsigned int cubemapTexture = loadCubemap(faces);

    vector<glm::vec3> vegetation
            {
                    glm::vec3(-1.5f, 0.0f, -0.48f),
                    glm::vec3( 1.5f, 0.0f, 0.51f),
                    glm::vec3( 0.0f, 0.0f, 0.7f),
                    glm::vec3(-0.3f, 0.0f, -2.3f),
                    glm::vec3 (0.5f, 0.0f, -0.6f)
            };

    // shader configuration
    // --------------------
    shader.use();
    shader.setInt("texture1", 0);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    BlinnPhongshader.use();
    BlinnPhongshader.setInt("blinn-phong", 0);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glm::vec3 lightPos(0.0f, 10.0f, 0.0f);
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
        // Postavite pointLight.position na poziciju kamere
        ourShader.setVec3("pointLight.position", programState->camera.Position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        ourShader.setBool("blinn",true);

        ourShader.setVec3("spotLight.position", programState->camera.Position );
        ourShader.setVec3("spotLight.direction", programState->camera.Front);
        ourShader.setVec3("spotLight.ambient", 0.4f, 0.3f, 0.3f);
        ourShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("spotLight.constant", 1.0f);
        ourShader.setFloat("spotLight.linear", 0.09);
        ourShader.setFloat("spotLight.quadratic", 0.032);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        ourShader.setVec3("dirLight.direction", glm::vec3(0.0f,-1.0f,0.0f));
        ourShader.setVec3("dirLight.ambient", glm::vec3(1.0f,1.0f,1.0f));
        ourShader.setVec3("dirLight.diffuse", glm::vec3(0.8f,0.8f,0.8f));
        ourShader.setVec3("dirLight.specular", glm::vec3(1.0f,1.0f,1.0f));

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);


        // render the bench model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3 (3,-0.48,1)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.005,0.005,0.005));
        model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);
        // render another bench model
        glm::mat4 model0 = glm::mat4(1.0f);
        model0 = glm::translate(model0,
                               glm::vec3 (5.750,-0.48,1.975)); // translate it down so it's at the center of the scene
        model0 = glm::scale(model0, glm::vec3(0.005,0.005,0.005));
        model0 = glm::rotate(model0, glm::radians(-20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model0);
        ourModel.Draw(ourShader);

        // Model drveta koji renderujemo

        glm::mat4 model1 = glm::mat4(1.0f);
        model1 = glm::translate(model1,
                                glm::vec3 (3,-0.43,3)); // translate it down so it's at the center of the scene
        model1 = glm::scale(model1, glm::vec3(0.25,0.2,0.25));
        model1 = glm::rotate(model1, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model1);
        treeModel.Draw(ourShader);

        //Model logorske vatre koji renderujemo

        glm::mat4 model3 = glm::mat4(1.0f);
        model3 = glm::translate(model3,
                                glm::vec3 (4,-0.094,2)); // translate it down so it's at the center of the scene
        model3 = glm::scale(model3, glm::vec3(0.5,0.5,0.5));
        model3 = glm::rotate(model3, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model3 = glm::rotate(model3, glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model3 = glm::rotate(model3, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model3);
        campFireModel.Draw(ourShader);

        /*glm::mat4 model4 = glm::mat4(1.0f);
        model4 = glm::translate(model4,
                                glm::vec3 (100,-15,27)); // translate it down so it's at the center of the scene
        model4 = glm::scale(model4, glm::vec3(0.5,0.5,0.5));
        model4 = glm::rotate(model4, glm::radians(13.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model4 = glm::rotate(model4, glm::radians(-7.6f), glm::vec3(1.0f, 0.0f, 0.0f));
        model4 = glm::rotate(model4, glm::radians(-10.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model4);

         grassModel.Draw(ourShader);
        */
        // model zbuna

        //glm::mat4 model5 = glm::mat4(1.0f);
        //model5 = glm::translate(model5,
          //                      glm::vec3 (100,-15,27)); // translate it down so it's at the center of the scene
        //model5 = glm::scale(model5, glm::vec3(5,5,5));
        //ourShader.setMat4("model", model5);
        //bushModel.Draw(ourShader);

        //blending
        blendingShader.use();
        blendingShader.setVec3("viewPosition", programState->camera.Position);
        blendingShader.setFloat("material.shininess", 32.0f);
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);
        blendingShader.setVec3("dirLight.direction", glm::vec3(-0.547f, -0.727f, 0.415f));
        blendingShader.setVec3("dirLight.ambient", glm::vec3(0.35f));
        blendingShader.setVec3("dirLight.diffuse", glm::vec3(0.4f));
        blendingShader.setVec3("dirLight.specular", glm::vec3(0.2f));


        // Model Sunca koji renderujemo
        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2, glm::vec3(10, 25, -10)); // translate it down so it's at the center of the scene
        model2 = glm::rotate(model2, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // Postavljanje veće osi rotacije
        glm::vec3 rotationCenter = glm::vec3(-6.57f, 10.0f, 10.0f); // Postavite centar rotacije na željenu točku
        glm::mat4 translateToOrigin = glm::translate(glm::mat4(1.0f), -rotationCenter);

        // Promjena rotacijske osi na veću os
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), (float)currentFrame / 3, glm::vec3(0.0f, 0.0f, 1.0f));

        // Rotacija sunca oko svoje osi
        glm::mat4 selfRotationMatrix = glm::rotate(glm::mat4(1.0f), (float)currentFrame / 1000, glm::vec3(0.0f, 1.0f, 0.0f));

        // Uvećavanje faktora skaliranja
        glm::vec3 scaleVector = glm::vec3(5.0f); // Promijenite faktor skaliranja prema potrebi
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scaleVector);

        // Ponovno postavljanje centra rotacije
        glm::mat4 translateBack = glm::translate(glm::mat4(1.0f), rotationCenter);

        // Kombinacija transformacija
        model2 = translateBack * scaleMatrix * selfRotationMatrix * rotationMatrix * translateToOrigin;

        model2 = glm::scale(model2, glm::vec3(0.05,0.05,0.05));
        // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model2);
        sunModel.Draw(ourShader);

        BlinnPhongshader.use();

        BlinnPhongshader.setMat4("projection", projection);
        BlinnPhongshader.setMat4("view", view);
        // set light uniforms
        BlinnPhongshader.setVec3("viewPos", programState->camera.Position);
        BlinnPhongshader.setVec3("lightPos", lightPos);
        BlinnPhongshader.setInt("blinn", blinn);
        // floor
        glBindVertexArray(planeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        // skybox cube

        skyboxShader.use();
        view[3][0] = 0; // Postavljam x translaciju na nulu
        view[3][1] = 0; // Postavljam y translaciju na nulu
        view[3][2] = 0; // postavljam z translaciju na nulu
        view[3][3] = 0;
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);

        blendingShader.setMat4("model", model2);
        sunModel.Draw(blendingShader);

        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        for (unsigned int i = 0; i < vegetation.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
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

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
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
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
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
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
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
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

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
}
