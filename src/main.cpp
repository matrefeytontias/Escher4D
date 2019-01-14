#define GLFW_DLL
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
// Define these only in *one* .cpp file.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <iostream>
#include <stdexcept>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"
#include <Eigen/Eigen>
// #include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Camera4.hpp"
#include "FSQuadRenderContext.hpp"
#include "OFFLoader.hpp"
#include "Object4.hpp"
#include "ShaderProgram.hpp"
#include "utils.hpp"

static void glfw_error_callback(int error, const char *description)
{
    trace("Error " << error << " : " << description);
}

static bool freezeCamera = false;
static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        freezeCamera = !freezeCamera;
        glfwSetInputMode(window, GLFW_CURSOR, freezeCamera ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
};

static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

// Deferred rendering buffers and textures
static GLuint gBuffer, dBuffer;
Texture *texPos, *texNorm, *texColor;

void setupDeferred(int w, int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    
    glBindTexture(GL_TEXTURE_2D, texPos->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texPos->id, 0);
    
    glBindTexture(GL_TEXTURE_2D, texNorm->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8_SNORM, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texNorm->id, 0);
    
    glBindTexture(GL_TEXTURE_2D, texColor->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texColor->id, 0);
    
    const static unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    
    glBindRenderbuffer(GL_RENDERBUFFER, dBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dBuffer);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    setupDeferred(width, height);
}

// demos.cpp
void complexDemo(Object4&);

Object4 Object4::scene;

int _main(int, char *argv[])
{
    setwd(argv);
    
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        trace("Couldn't initialize GLFW");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(1280, 720, "GLFW Window", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1); // Enable vsync
    
    // Setup ImGui binding
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    ImGui_ImplGlfwGL3_Init(window, true);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    
    // Setup style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();
    
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    
    glEnable(GL_DEPTH_TEST);
    
    Camera4 camera(window);
    camera.pos << 0, 1.5, 0, 0;
    // camera.pos(2) = 5;
    
    Eigen::Matrix4f p(Eigen::Matrix4f::Identity());
    ShaderProgram program;
    program.attach(GL_VERTEX_SHADER, "shaders/vertex.glsl");
    program.attach(GL_GEOMETRY_SHADER, "shaders/geometry.glsl");
    program.attach(GL_FRAGMENT_SHADER, "shaders/fragment.glsl");
    
    // Load geometry
    std::vector<Eigen::Vector3f> vertices;
    std::vector<unsigned int> tris, tetras;
    // Cube
    if(!OFFLoader::loadModel("models/cube", vertices, tris, tetras))
    {
        trace("Can't load that shizzle");
        return 0;
    }
    Geometry4 cubeGeometry = Geometry4::from3D(vertices, tetras);
    for(unsigned int k = 0; k < cubeGeometry.vertices.size(); k++)
        cubeGeometry.normals.push_back(Vector4f(0, 0, 0, -1));
    cubeGeometry.uploadGPU();
    Model4RenderContext cubeRC(cubeGeometry, program);
    // Holed cube
    if(!OFFLoader::loadModel("models/holedCube", vertices, tris, tetras))
    {
        trace("Can't load that shizzle");
        return 0;
    }
    Geometry4 holedGeometry = Geometry4::from3D(vertices, tetras);
    for(unsigned int k = 0; k < holedGeometry.vertices.size(); k++)
        holedGeometry.normals.push_back(Vector4f(0, 0, 0, -1));
    holedGeometry.uploadGPU();
    Model4RenderContext holedRC(holedGeometry, program);
    
    // Build the complex
    Object4 &scene = Object4::scene;
    // Build a single room
    // 3 cells have a hole in them, respectively in the directions +X, +Z and +W
    Object4 &room1 = scene.addChild();
    for(unsigned int k = 0; k < 5; k++)
    {
        room1.addChild(cubeRC);
        room1[k].color << 1, 1, 1, 1;
    }
    room1.addChild(holedRC);
    room1[5].color << 1, 0, 0, 1;
    room1.addChild(holedRC);
    room1[6].color << 0, 1, 0, 1;
    room1.addChild(holedRC);
    room1[7].color << 0, 0, 1, 1;
    
    // +X
    room1[5].pos(0) = .5;
    room1[5].rotate(XW, -M_PI / 2);
    // -X
    room1[0].pos(0) = -.5;
    room1[0].rotate(XW, M_PI / 2);
    // +Y
    room1[1].pos(1) = .5;
    room1[1].rotate(YW, -M_PI / 2);
    // -Y
    room1[2].pos(1) = -.5;
    room1[2].rotate(YW, M_PI / 2);
    // +Z
    room1[6].pos(2) = .5;
    room1[6].rotate(ZW, -M_PI / 2);
    // -Z
    room1[3].pos(2) = -.5;
    room1[3].rotate(ZW, M_PI / 2);
    // +W
    room1[7].pos(3) = .5;
    // -W
    room1[4].pos(3) = -.5;
    room1[4].rotate(XW, M_PI); // 180° rotation, any axis + W
    
    complexDemo(scene);
    
    // Print amount of tetrahedra for fun
    {
        int tetrahedra = 0;
        scene.visit([&tetrahedra](const Object4 &obj)
        {
            const Model4RenderContext *rc = obj.getRenderContext();
            if(rc)
                tetrahedra += rc->geometry.cells.size() / 4;
        });
        trace("Scene has " << tetrahedra << " tetrahedra.");
    }
    
    
    perspective(p, 90, (float)display_w / display_h, 0.0001, 100);
    
    trace("Entering drawing loop");
    
    float lightIntensity = 10.f, lightRadius = 20;
    
    float timeBase = glfwGetTime();
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    scene.scale(Vector4f(10, 6, 10, 10)).pos(1) = 3;
    
    // Setup deferred shading
    ShaderProgram quadProgram;
    quadProgram.attach(GL_VERTEX_SHADER, "shaders/deferred_vert.glsl");
    quadProgram.attach(GL_FRAGMENT_SHADER, "shaders/deferred_frag.glsl");
    texPos = &quadProgram.getTexture("texPos");
    texNorm = &quadProgram.getTexture("texNorm");
    texColor = &quadProgram.getTexture("texColor");
    
    glGenFramebuffers(1, &gBuffer);
    glGenRenderbuffers(1, &dBuffer);
    
    setupDeferred(display_w, display_h);
    
    FSQuadRenderContext quadRC(quadProgram);
    
    glClearColor(0, 0, 0, 1);
    
    while (!glfwWindowShouldClose(window))
    {
        glfwGetFramebufferSize(window, &display_w, &display_h);
        setAspectRatio(p, (float)display_w / display_h);
        
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        float now = glfwGetTime(), dt = now - timeBase;
        timeBase = now;
        
        program.use();
        
        program.uniformMatrix4fv("P", 1, &p.data()[0]);
        program.uniform1f("uLightIntensity", lightIntensity);
        program.uniform1f("uLightRadius", lightRadius);
        
        scene.render(camera);
        
        // Deferred rendering
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        quadProgram.use();
        quadRC.render();
        
        ImGui_ImplGlfwGL3_NewFrame();
        
        ImGui::Begin("Test parameters", NULL, ImGuiWindowFlags_AlwaysAutoResize);
            if(ImGui::TreeNode("Lighting parameters"))
            {
                ImGui::SliderFloat("Light intensity", &lightIntensity, 0, 20);
                ImGui::SliderFloat("Light radius", &lightRadius, 1, 50);
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Camera control"))
            {
                ImGui::SliderFloat("Movement speed", &camera.speed, 1, 10);
                ImGui::SliderFloat("Rotation divisor X", &camera.rotationDivisorX, 1, display_w);
                ImGui::SliderFloat("Rotation divisor Y", &camera.rotationDivisorY, 1, display_h);
                ImGui::SliderFloat("XW+ZW rotation speed", &camera.xwzwSpeed, 0.1, (float)M_PI * 2);
                ImGui::TreePop();
            }
        ImGui::End();
        
        ImGui::Begin("Debug info", NULL, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
            ImGui::Text("Camera position : %lf, %lf, %lf, %lf",
                camera.pos(0), camera.pos(1), camera.pos(2), camera.pos(3));
            ImGui::Text("Camera rotation : %lf, %lf, %lf", camera._xz, camera._yz, camera._xwzw);
        ImGui::End();
                
        ImGui::Render();
        ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
        if(!freezeCamera)
            camera.update(dt);
    }
    
    trace("Exiting drawing loop");
    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    
    return 0;
}

int main(int argc, char *argv[])
{
    try
    {
        return _main(argc, argv);
    }
    catch(std::exception &e)
    {
        std::cerr << e.what();
        throw;
    }
}
