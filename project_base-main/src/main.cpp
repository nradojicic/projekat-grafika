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

void renderWall();
void renderQuad();
unsigned int loadCubemap(vector<std::string> faces);

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
bool blinn = false;
bool hdr = false;
bool hdrKeyPressed = false;
bool bloom = false;
bool bloomKeyPressed = false;
float exposure = 1.0f;


// timing

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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Park", NULL, NULL);
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
    //Shader wallShader("resources/shaders/wall.vs", "resources/shaders/wall.fs");
    Shader shader("resources/shaders/cubemaps.vs", "resources/shaders/cubemaps.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader blendingShader("resources/shaders/2.model_lighting.vs", "resources/shaders/blending.fs" );
    Shader BlinnPhongshader("resources/shaders/1.advanced_lighting.vs", "resources/shaders/1.advanced_lighting.fs");
    Shader travaShader("resources/shaders/3.1.blending.vs","resources/shaders/3.1.blending.fs");
    Shader HdrShader("resources/shaders/hdr.vs", "resources/shaders/hdr.fs");
    Shader bloomShader("resources/shaders/bloom.vs", "resources/shaders/bloom.fs");
    Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    // load models
    // -----------

    Model ourModel("resources/objects/klupa3/uploads_files_793049_Bank-of-Wooden;OBJ;FBX/Bank-of-Wooden;OBJ;FBX/Bank_of_Wooden.obj");
    ourModel.SetShaderTextureNamePrefix("material.");

    Model benchModel("resources/objects/klupa3/uploads_files_793049_Bank-of-Wooden;OBJ;FBX/Bank-of-Wooden;OBJ;FBX/Bank_of_Wooden.obj");
    benchModel.SetShaderTextureNamePrefix("material.");

    Model sunModel("resources/objects/sunce/Sun.obj");
    sunModel.SetShaderTextureNamePrefix("material.");


    stbi_set_flip_vertically_on_load(false);
    Model drvecaModel("resources/objects/park1/uploads_files_3749963_tree.obj");
    drvecaModel.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    /*
    stbi_set_flip_vertically_on_load(false);
    Model grassModel("resources/objects/seesaw/uploads_files_4642818_SeeSaw/Obj/SeaSaw.obj");
    grassModel.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);
    */

    Model swingModel("resources/objects/SWING2/untitled.obj");
    swingModel.SetShaderTextureNamePrefix("material.");

    Model toboganModel("resources/objects/tobogan/tobogan.obj");
    toboganModel.SetShaderTextureNamePrefix("material.");

    Model treeModel("resources/objects/tree5/uploads_files_2418161_ItalianCypress/ItalianCypress.obj");
    treeModel.SetShaderTextureNamePrefix("material.");

    /*
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/bricks2.jpg").c_str());
    unsigned int normalMap  = loadTexture(FileSystem::getPath("resources/textures/bricks2_normal.jpg").c_str());
    unsigned int heightMap  = loadTexture(FileSystem::getPath("resources/textures/bricks2_disp.jpg").c_str());
    */
    float planeVertices[] = {
            // positions            // normals         // texcoords
            20.0f, -0.5f, -20.0f,  0.0f, 1.0f, 0.0f,  20.0f, 20.0f, // Vertex 6
            -20.0f, -0.5f, -20.0f,  0.0f, 1.0f, 0.0f,   0.0f, 20.0f, // Vertex 5
            20.0f, -0.5f,  20.0f,  0.0f, 1.0f, 0.0f,  20.0f,  0.0f, // Vertex 4

            -20.0f, -0.5f, -20.0f,  0.0f, 1.0f, 0.0f,   0.0f, 20.0f, // Vertex 3
            -20.0f, -0.5f,  20.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f, // Vertex 2
            20.0f, -0.5f,  20.0f,  0.0f, 1.0f, 0.0f,  20.0f,  0.0f  // Vertex 1
    };
    float sideVertices[] = {
            // positions            // normals         // texcoords

            // Desni zid
            20.0f, -4.0f, -20.0f,  1.0f, 0.0f, 0.0f,  0.0f,  0.0f,
            20.0f, -4.0f,  20.0f,  1.0f, 0.0f, 0.0f,  20.0f, 0.0f,
            20.0f,  4.0f,  20.0f,  1.0f, 0.0f, 0.0f,  20.0f, 1.0f,
            20.0f, -4.0f, -20.0f,  1.0f, 0.0f, 0.0f,  0.0f,  0.0f,
            20.0f,  4.0f,  20.0f,  1.0f, 0.0f, 0.0f,  20.0f, 1.0f,
            20.0f,  4.0f, -20.0f,  1.0f, 0.0f, 0.0f,  0.0f,  1.0f,
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
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f,
            2.0f,  0.5f,  0.0f,  1.0f,  0.0f,
            3.0f,  0.5f,  0.0f,  1.0f,  0.0f,
            4.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.2, 0.2, 0.2);
    pointLight.specular = glm::vec3(0.01, 0.01, 0.01);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.000001f;
    pointLight.quadratic = 0.0000001f;

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
    // side VAO

    unsigned int sideVAO, sideVBO;
    glGenVertexArrays(1, &sideVAO);
    glGenBuffers(1, &sideVBO);
    glBindVertexArray(sideVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sideVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sideVertices), sideVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    //load textures
    stbi_set_flip_vertically_on_load(false);
    unsigned int GrassTexture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str());
    stbi_set_flip_vertically_on_load(true);
    unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/textures/classic-green-grass-seamless-texture-free-photo.png").c_str());
    unsigned int sideTexture = loadTexture(FileSystem::getPath("resources/textures/bricks2.jpg").c_str());

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

    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // preparing blur.
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    vector<glm::vec3> vegetation
            {
                    glm::vec3(-1.5f, 0.0f, -0.48f),
                    glm::vec3( 1.5f, 0.0f, 0.51f),
                    glm::vec3( 0.0f, 0.0f, 0.7f),
                    glm::vec3(-0.3f, 0.0f, -2.3f),
                    glm::vec3 (0.5f, 0.0f, -0.6f),
                    glm::vec3(2,0,3),
                    glm::vec3(2,0,4),
                    glm::vec3(2,0,2),
                    glm::vec3(2,0,0),


            };

    // shader configuration
    // --------------------
    shader.use();
    shader.setInt("texture1", 0);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    //HdrShader.use();
    //HdrShader.setInt("hdrBuffer", 0);
    //HdrShader.setInt("bloomBlur", 1);

    blurShader.use();
    blurShader.setInt("image", 0);

    bloomShader.use();
    bloomShader.setInt("scene", 0);
    bloomShader.setInt("bloomBlur", 1);

    BlinnPhongshader.use();
    BlinnPhongshader.setInt("blinn-phong", 0);

    /*
    wallShader.use();
    wallShader.setInt("diffuseMap", 0);
    wallShader.setInt("normalMap", 1);
    wallShader.setInt("depthMap", 2);
    */
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glm::vec3 lightPos(0.0f, 4.0f, 3.0f);
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
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
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
        ourShader.setVec3("material.specular", 0.0f, 0.0f, 0.0f);
        ourShader.setBool("blinn",false);

        ourShader.setVec3("spotLight.position", programState->camera.Position );
        ourShader.setVec3("spotLight.direction", programState->camera.Front);
        ourShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight.diffuse", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("spotLight.constant", 1.0f);
        ourShader.setFloat("spotLight.linear", 0.09);
        ourShader.setFloat("spotLight.quadratic", 0.032);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));


        ourShader.setVec3("dirLight.position", glm::vec3(1.0,2.0,3.0));
        ourShader.setVec3("dirLight.direction", glm::vec3(0.0f,-1.0f,0.0f));
        ourShader.setVec3("dirLight.ambient", glm::vec3(0.1f,0.1f,0.1f));
        ourShader.setVec3("dirLight.diffuse", glm::vec3(0.1f,0.1f,0.1f));
        ourShader.setVec3("dirLight.specular", glm::vec3(0.1f,0.1f,0.1f));

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
                                glm::vec3 (4,-0.55,2)); // translate it down so it's at the center of the scene
        model3 = glm::scale(model3, glm::vec3(0.5,0.5,0.5));
        //model3 = glm::rotate(model3, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        //model3 = glm::rotate(model3, glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        //model3 = glm::rotate(model3, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model3);
        drvecaModel.Draw(ourShader);


        //tobogan
        glm::mat4 modelTobogan = glm::mat4(1.0f);
        modelTobogan = glm::translate(modelTobogan,
                                      glm::vec3 (-2.50,-0.45,3.0)); // translate it down so it's at the center of the scene
        modelTobogan = glm::scale(modelTobogan, glm::vec3(0.5,0.5,0.5));

        // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelTobogan);
        toboganModel.Draw(ourShader);


        glm::mat4 modelSwing = glm::mat4(1.0f);
        modelSwing = glm::translate(modelSwing ,
                                     glm::vec3 (0,-0.6,1.2)); // translate it down so it's at the center of the scene
        modelSwing  = glm::scale(modelSwing , glm::vec3(0.23,0.23,0.27));


        // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelSwing);
        swingModel.Draw(ourShader);


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
        //zid

        glBindVertexArray(sideVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sideTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content


        travaShader.use();
        travaShader.setMat4("projection", projection);
        travaShader.setMat4("view", view);

        glDisable(GL_CULL_FACE);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        for (unsigned int i = 0; i < vegetation.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        /*
        wallShader.use();
        wallShader.setMat4("projection", projection);
        wallShader.setMat4("view", view);
        wallShader.setVec3("viewPos", programState->camera.Position);
        model = glm::mat4(1.0f);
        wallShader.setMat4("model", model);
        wallShader.setFloat("height_scale", heightScale);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, heightMap);

        renderWall();
        glDisable(GL_CULL_FACE);
        */
        //poslednji skybox
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

        // blur
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        blurShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // --------------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //bloomShader.use();
        HdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        HdrShader.setBool("hdr", hdr);
        HdrShader.setInt("bloom", bloom);
//        bloomShader.setInt("bloom", bloom);
//        bloomShader.setFloat("exposure", exposure);
        HdrShader.setFloat("exposure", exposure);
        renderQuad();

        std::cout << "hdr: " << (hdr ? "on" : "off") << std::endl;
        std::cout << "bloom: " << (bloom ? "on" : "off") << " | exposure: " << exposure << std::endl;


        blendingShader.setMat4("model", model2);
        sunModel.Draw(blendingShader);


        if (programState->ImGuiEnabled)
            //DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);

    //glDeleteVertexArrays(1, &sideVAO);
    //glDeleteBuffers(1, &sideVBO);

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

unsigned int wallVAO = 0;
unsigned int wallVBO;


void renderWall() {
    GLuint wallVAO = 0;
    GLuint wallVBO = 0;

    if (wallVAO == 0) {
        glm::vec3 pos1(20.0f, -4.0f, -20.0f);
        glm::vec3 pos2(20.0f, -4.0f,  20.0f);
        glm::vec3 pos3(20.0f,  4.0f,  20.0f);
        glm::vec3 pos4(20.0f, 4.0f, -20.0f);

        glm::vec2 uv1(0.0f, 0.0f);
        glm::vec2 uv2(20.0f, 0.0f);
        glm::vec2 uv3(20.0f, 1.0f);
        glm::vec2 uv4(0.0f, 0.0f);

        glm::vec3 nm(0.0f, 1.0f, 0.0f);

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

        float wallVertices[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        glGenVertexArrays(1, &wallVAO);
        glGenBuffers(1, &wallVBO);

        glBindVertexArray(wallVAO);
        glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), &wallVertices, GL_STATIC_DRAW);

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

    glBindVertexArray(wallVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
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

    if (programState->ImGuiEnabled)
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
    if(key == GLFW_KEY_B && action == GLFW_PRESS){
        blinn = !blinn;}

    if(key == GLFW_KEY_H && action == GLFW_PRESS){
        hdr = true;
        hdrKeyPressed = true;
    }

    if(key==GLFW_KEY_H && action == GLFW_RELEASE){
        hdrKeyPressed = false;
        hdr = false;
    }

    if(key == GLFW_KEY_J && action == GLFW_PRESS){
        bloom = true;
        bloomKeyPressed = true;
    }

    if(key == GLFW_KEY_J && action == GLFW_RELEASE){
        bloom = false;
        bloomKeyPressed = false;
    }
}
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}