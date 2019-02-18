#define GLFW_DLL
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
// Define these only in *one* .cpp file.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <algorithm>
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
#include "HierarchicalBuffer.hpp"
#include "OFFLoader.hpp"
#include "Object4.hpp"
#include "ShaderProgram.hpp"
#include "ShadowHypervolumes.hpp"
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
static Texture *texPos, *texNorm, *texColor;

/**
 * Sets up internals (framebuffer and textures) for deferred rendering given
 * framebuffer dimensions.
 */
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texColor->id, 0);
    
    const static unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    
    glBindRenderbuffer(GL_RENDERBUFFER, dBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    setupDeferred(width, height);
}

static void printGLerror(GLenum, GLenum type, GLuint, GLenum severity,
    GLsizei, const GLchar *message, const void*)
{
    std::cerr << "GL callback :" << (type == GL_DEBUG_TYPE_ERROR ? " error" : "")
        << " type = 0x" << std::hex << type << "(0x" << severity << ") : " << std::dec
        << message << std::endl;
}

// demos.cpp
void complexDemo(Object4&);

Object4 Object4::scene;

extern "C" {
    /**
     * Tell the Nvidia driver to make itself useful.
     */
    
    #ifdef linux
    __attribute__((visibility("default")))
    #else
    __declspec(dllexport)
    #endif
     uint64_t NvOptimusEnablement = 0x00000001;
}

int _main(int, char *argv[])
{
    setwd(argv);
    
    /// Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        trace("Couldn't initialize GLFW");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    GLFWwindow *window = glfwCreateWindow(1280, 720, "GLFW Window", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(0); // no v-sync, live on the edge
    
    glEnable(GL_DEBUG_OUTPUT);
    glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(printGLerror, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, NULL, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_FALSE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    
    /// Setup ImGui binding
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    ImGui_ImplGlfwGL3_Init(window, true);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    
    /// Setup style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();
    
    /// Scene setup
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    
    glEnable(GL_DEPTH_TEST);
    
    Camera4 camera(window);
    camera.pos << 0, 1.5, 0, 0;
    
    Eigen::Matrix4f p(Eigen::Matrix4f::Identity());
    ShaderProgram program;
    program.attach(GL_VERTEX_SHADER, "shaders/vertex.glsl");
    program.attach(GL_GEOMETRY_SHADER, "shaders/geometry.glsl");
    program.attach(GL_FRAGMENT_SHADER, "shaders/fragment.glsl");
    
    // Load geometry
    Geometry4 cubeGeometry, holedGeometry, stoolGeometry;
    {
        std::vector<Eigen::Vector3f> vertices;
        std::vector<unsigned int> tris, tetras;
        // Cube
        if(!OFFLoader::loadModel("models/cube", vertices, tris, tetras))
        {
            trace("Can't load that shizzle");
            return 0;
        }
        cubeGeometry.from3D(vertices, tris, tetras, 0.1);
        cubeGeometry.unindex();
        cubeGeometry.recomputeNormals(true);
        cubeGeometry.uploadGPU();
        // Holed cube
        if(!OFFLoader::loadModel("models/holedCube", vertices, tris, tetras))
        {
            trace("Can't load that shizzle");
            return 0;
        }
        holedGeometry.from3D(vertices, tris, tetras, 0.1);
        holedGeometry.unindex();
        holedGeometry.recomputeNormals(true);
        holedGeometry.uploadGPU();
        
        // Stool 3D model
        if(!OFFLoader::loadModel("models/stool", vertices, tris, tetras))
        {
            trace("No stool for you");
            return 0;
        }
        stoolGeometry.from3D(vertices, tris, tetras, 6.);
        stoolGeometry.recomputeNormals();
        stoolGeometry.uploadGPU();
    }
    Model4RenderContext cubeRC(cubeGeometry, program), holedRC(holedGeometry, program),
        stoolRC(stoolGeometry, program);
    
    // Build the complex
    Object4 &complex = Object4::scene.addChild();
    {
        // Build a single room
        // 3 cells have a hole in them, respectively in the directions +X, +Z and +W
        Object4 &room1 = complex.addChild();
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
    }
    
    complexDemo(complex);
    
    Object4 &stool = Object4::scene.addChild(stoolRC);
    stool.rotate(YZ, M_PI / 2).scale(Vector4f(0.075, 0.075, 0.075, 0.075)).pos << 0, 1, -4, 0;
    stool.color << 1, 1, 1, 1;
    
    // Print amount of tetrahedra for fun
    int tetrahedra = 0;
    Object4::scene.visit([&tetrahedra](const Object4 &obj)
    {
        const Model4RenderContext *rc = obj.getRenderContext();
        if(rc)
        {
            if(rc->geometry.isIndexed())
                tetrahedra += rc->geometry.cells.size() / 4;
            else
                tetrahedra += rc->geometry.vertices.size() / 4;
        }
    });
    
    perspective(p, 90, (float)display_w / display_h, 0.01, 40);
    
    float lightIntensity = 10.f, lightRadius = 20;
    
    float timeBase = glfwGetTime();
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    complex.scale(Vector4f(10, 6, 10, 10)).pos(1) = 3;
    
    /// Setup deferred shading
    ShaderProgram quadProgram;
    quadProgram.attach(GL_VERTEX_SHADER, "shaders/deferred_vert.glsl");
    quadProgram.attach(GL_FRAGMENT_SHADER, "shaders/deferred_frag.glsl");
    texPos = &quadProgram.getTexture("texPos");
    texNorm = &quadProgram.getTexture("texNorm");
    texColor = &quadProgram.getTexture("texColor");
    
    glGenFramebuffers(1, &gBuffer);
    glGenRenderbuffers(1, &dBuffer);
    
    FSQuadRenderContext quadRC(quadProgram);
    
    /// Setup shadow hypervolumes computations
    ShadowHypervolumes svComputer;
    
    std::vector<int> cellsCompBuffer; // ivec4
    std::vector<int> objIndexCompBuffer; // int
    std::vector<Vector4f> vertexCompBuffer; // vec4
    std::vector<Matrix4f> MCompBuffer; // mat4
    std::vector<Vector4f> MtCompBuffer; // vec4
    
    int objectIndex = 0;
    Object4::scene.visit([&](const Object4 &obj)
    {
        if(obj.castShadows)
        {
            const Model4RenderContext *rc = obj.getRenderContext();
            if(rc)
            {
                const Geometry4 &geom = rc->geometry;
                const int vertexCount = vertexCompBuffer.size();
                if(geom.isIndexed())
                {
                    for(unsigned int k = 0; k < geom.cells.size(); k++)
                    {
                        cellsCompBuffer.push_back(geom.cells[k] + vertexCount);
                        if(!(k % 4))
                            objIndexCompBuffer.push_back(objectIndex);
                    }
                }
                else
                {
                    for(unsigned int k = 0; k < geom.vertices.size(); k++)
                    {
                        cellsCompBuffer.push_back(k + vertexCount);
                        if(!(k % 4))
                            objIndexCompBuffer.push_back(objectIndex);
                    }
                }
                vertexCompBuffer.insert(vertexCompBuffer.end(), geom.vertices.begin(), geom.vertices.end());
            }
        }
        objectIndex++;
    });
    
    svComputer.reinit(display_w, display_h, *texPos, cellsCompBuffer, objIndexCompBuffer, vertexCompBuffer);
    
    /// Start draw loop
    {
        int mwgcx, mwgcy, mwgcz, msms;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &mwgcx);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &mwgcy);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &mwgcz);
        glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &msms);
        trace("GPU allows for a maximum of (" << mwgcx << ", " << mwgcy << ", " << mwgcz << ") work groups");
        trace("GPU allows for " << msms << " bytes of shared memory");
    }
    
    setupDeferred(display_w, display_h);
    
    glClearColor(0, 0, 0, 1);
    
    trace("Entering drawing loop");
    
    while (!glfwWindowShouldClose(window))
    {
        glfwGetFramebufferSize(window, &display_w, &display_h);
        setAspectRatio(p, (float)display_w / display_h);
        
        /// Render scene on framebuffer for deferred shading
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        float now = glfwGetTime(), dt = now - timeBase;
        timeBase = now;
        
        Vector4f lightPos;
        Transform4 vt = camera.computeViewTransform();
        lightPos << sin(now) * 2, sin(now * 1.5) * 1.5 + 2, 0, cos(now) * 2;
        lightPos = vt.apply(lightPos);
        // lightPos << 0.5, 0, 0.5, 0;
        
        program.use();
        
        program.uniformMatrix4fv("P", 1, &p.data()[0]);
        
        Object4::scene.render(camera);
        
        /// GPGPU fun
        // Generate AABB hierarchy and bind test program
        ShaderProgram &computeProgram = svComputer.precompute();
        
        // Compute M transforms (matrix + translation) for all meshes
        MCompBuffer.clear();
        MtCompBuffer.clear();
        {
            Transform4 dummy;
            Object4::scene.visit<Transform4>([&](const Object4 &obj, Transform4 &pm)
            {
                Transform4 m = obj.chain(pm);
                MCompBuffer.push_back(m.mat);
                MtCompBuffer.push_back(m.pos);
                return m;
            }, dummy);
        }
        
        // Bind textures and whatnot
        glBindImageTexture(0, texPos->id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        computeProgram.uniform2i("uTexSize", display_w, display_h);
        computeProgram.uniformMatrix4fv("V", 1, vt.mat.data());
        computeProgram.uniform4f("Vt", vt.pos(0), vt.pos(1), vt.pos(2), vt.pos(3));
        // Perform the actual computation
        computeProgram.uniform4f("uLightPos", lightPos(0), lightPos(1), lightPos(2), lightPos(3));
        svComputer.compute(MCompBuffer, MtCompBuffer);
        
        /// Deferred rendering
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        quadProgram.use();
        quadProgram.uniform1f("uLightIntensity", lightIntensity);
        quadProgram.uniform1f("uLightRadius", lightRadius);
        quadProgram.uniform4f("uLightPos", lightPos(0), lightPos(1), lightPos(2), lightPos(3));
        quadProgram.uniform2i("uTexSize", display_w, display_h);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
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
            ImGui::Text("Rendering %d tetrahedra at %.1f FPS", tetrahedra, ImGui::GetIO().Framerate);
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
    
    glDeleteRenderbuffers(1, &dBuffer);
    glDeleteFramebuffers(1, &gBuffer);
    
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
