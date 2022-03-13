// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
// Define these only in *one* .cpp file.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Empty/Context.hpp>
#include <Empty/math/vec.h>
#include <Empty/math/mat.h>

#include "Camera4.hpp"
#include "Context.h"
#include "FSQuadRenderContext.hpp"
#include "HierarchicalBuffer.hpp"
#include "OFFLoader.hpp"
#include "Object4.hpp"
#include "ShaderProgram.hpp"
#include "ShadowHypervolumes.hpp"
#include "utils.hpp"

Context Context::_instance;

// demos.cpp
void complexDemo(Object4&);

Object4 Object4::scene;

static void printGLerror(Empty::gl::DebugMessageSource, Empty::gl::DebugMessageType type, Empty::gl::DebugMessageSeverity severity, int,
                         const std::string& message, const void*)
{
    std::cerr << "GL " << Empty::utils::name(type) << " (" << Empty::utils::name(severity) << ") : "
        << message << std::endl;
}

int _main(int, char *argv[])
{
    Context& context = Context::get();
    context.init("Escher4D demo", 1280, 720);

    {
        using namespace Empty::gl;
        context.enable(ContextCapability::DebugOutput);
        context.disable(ContextCapability::DebugOutputSynchronous);
        context.debugMessageCallback(printGLerror, nullptr);
        context.debugMessageControl(DebugMessageSource::DontCare, DebugMessageType::DontCare, DebugMessageSeverity::High, true);
        context.debugMessageControl(DebugMessageSource::DontCare, DebugMessageType::DontCare, DebugMessageSeverity::Medium, true);
        context.debugMessageControl(DebugMessageSource::DontCare, DebugMessageType::DontCare, DebugMessageSeverity::Low, false);
        context.debugMessageControl(DebugMessageSource::DontCare, DebugMessageType::DontCare, DebugMessageSeverity::Notification, false);
    }

    context.setViewport(context.frameWidth, context.frameHeight);
    
    context.enable(Empty::gl::ContextCapability::DepthTest);
    
    Camera4 camera(context.window);
    camera.pos = Empty::math::vec4(0.f, 1.5f, 0.f, 0.f);
    
    Empty::math::mat4 p = Empty::math::mat4::Identity();
    ShaderProgram program;
    program.attach(GL_VERTEX_SHADER, "shaders/vertex.glsl");
    program.attach(GL_GEOMETRY_SHADER, "shaders/geometry.glsl");
    program.attach(GL_FRAGMENT_SHADER, "shaders/fragment.glsl");
    
    // Load geometry
    Geometry4 cubeGeometry, holedGeometry;
    {
        std::vector<Empty::math::vec3> vertices;
        std::vector<unsigned int> tris, tetras;
        // Cube
        if(!OFFLoader::loadModel("models/cube", vertices, tris, tetras))
        {
            trace("Can't load that shizzle");
            return 0;
        }
        cubeGeometry.from3D(vertices, tris, tetras, 0.1f);
        cubeGeometry.unindex();
        cubeGeometry.recomputeNormals(true);
        cubeGeometry.uploadGPU();
        // Holed cube
        if(!OFFLoader::loadModel("models/holedCube", vertices, tris, tetras))
        {
            trace("Can't load that shizzle");
            return 0;
        }
        holedGeometry.from3D(vertices, tris, tetras, 0.1f);
        holedGeometry.unindex();
        holedGeometry.recomputeNormals(true);
        holedGeometry.uploadGPU();
    }
    Model4RenderContext cubeRC(cubeGeometry, program), holedRC(holedGeometry, program);
    
    // Build the complex
    Object4 &complex = Object4::scene.addChild();
    {
        // Build a single room
        // 3 cells have a hole in them, respectively in the directions +X, +Z and +W
        Object4 &room1 = complex.addChild();
        for(unsigned int k = 0; k < 5; k++)
        {
            room1.addChild(cubeRC);
            room1[k].color = Empty::math::vec4(1, 1, 1, 1);
        }
        room1.addChild(holedRC);
        room1[5].color = Empty::math::vec4(1, 0, 0, 1);
        room1.addChild(holedRC);
        room1[6].color = Empty::math::vec4(0, 1, 0, 1);
        room1.addChild(holedRC);
        room1[7].color = Empty::math::vec4(0, 0, 1, 1);
        
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
    
    Object4 &cube = Object4::scene.addChild(cubeRC);
    cube.scale(Empty::math::vec4(0.5f, 0.5f, 0.5f, 5.f)).pos = Empty::math::vec4(0, 2, 5, 0);
    cube.color = Empty::math::vec4(1, 1, 1, 1);
    cube.insideOut = true;
    
    // Print amount of tetrahedra for fun
    int tetrahedra = 0;
    Object4::scene.visit([&tetrahedra](const Object4 &obj)
    {
        const Model4RenderContext *rc = obj.getRenderContext();
        if(rc)
        {
            tetrahedra += static_cast<int>(rc->geometry.isIndexed() ? rc->geometry.cells.size() / 4
                : rc->geometry.vertices.size() / 4);
        }
    });
    
    perspective(p, 90, (float)context.frameWidth / context.frameHeight, 0.01f, 40.f);
    
    float lightIntensity = 10.f, lightRadius = 20;
    
    float timeBase = static_cast<float>(glfwGetTime());
    
    glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    complex.scale(Empty::math::vec4(10, 6, 10, 10)).pos(1) = 3;
    
    /// Setup deferred shading
    ShaderProgram quadProgram;
    quadProgram.attach(GL_VERTEX_SHADER, "shaders/deferred_vert.glsl");
    quadProgram.attach(GL_FRAGMENT_SHADER, "shaders/deferred_frag.glsl");
    context.texPos = &quadProgram.getTexture("texPos");
    context.texNorm = &quadProgram.getTexture("texNorm");
    context.texColor = &quadProgram.getTexture("texColor");
    
    FSQuadRenderContext quadRC(quadProgram);
    
    /// Setup shadow hypervolumes computations
    ShadowHypervolumes svComputer;
    
    std::vector<int> cellsCompBuffer; // ivec4
    std::vector<int> objIndexCompBuffer; // int
    std::vector<Empty::math::vec4> vertexCompBuffer; // vec4
    std::vector<Empty::math::mat4> MCompBuffer; // mat4
    std::vector<Empty::math::vec4> MtCompBuffer; // vec4
    
    int objectIndex = 0;
    Object4::scene.visit([&](const Object4 &obj)
    {
        if(obj.castShadows)
        {
            const Model4RenderContext *rc = obj.getRenderContext();
            if(rc)
            {
                const Geometry4 &geom = rc->geometry;
                const int vertexCount = static_cast<int>(vertexCompBuffer.size());
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
    
    svComputer.reinit(context.frameWidth, context.frameHeight, *context.texPos, cellsCompBuffer, objIndexCompBuffer, vertexCompBuffer);
    
    /// Start draw loop
    {
        using namespace Empty::gl;
        int mwgcx = context.getStateVar<ContextStateVar::MaxComputeWorkGroupCount>(0),
            mwgcy = context.getStateVar<ContextStateVar::MaxComputeWorkGroupCount>(1),
            mwgcz = context.getStateVar<ContextStateVar::MaxComputeWorkGroupCount>(2),
            msms = context.getStateVar<ContextStateVar::MaxComputeSharedMemorySize>();
        trace("GPU allows for a maximum of (" << mwgcx << ", " << mwgcy << ", " << mwgcz << ") work groups");
        trace("GPU allows for " << msms << " bytes of shared memory");
    }
    
    context.setupDeferred(context.frameWidth, context.frameHeight);
    
    glClearColor(0, 0, 0, 1);
    
    trace("Entering drawing loop");
    
    setAspectRatio(p, (float)context.frameWidth / context.frameHeight);
    
    while (!glfwWindowShouldClose(context.window))
    {
        context.newFrame();

        /// Render scene on framebuffer for deferred shading
        glBindFramebuffer(GL_FRAMEBUFFER, context.gBuffer->getInfo());
        context.setViewport(context.frameWidth, context.frameHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        float now = static_cast<float>(glfwGetTime()), dt = now - timeBase;
        timeBase = now;
        
        Empty::math::vec4 lightPos(sin(now) * 2.f, sin(now * 1.5f) * 1.5f + 2.f, 0.f, cos(now) * 2.f);
        Transform4 vt = camera.computeViewTransform();
        lightPos = vt.apply(lightPos);
        
        program.use();
        
        program.uniformMatrix4fv("P", 1, p);
        
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
        glBindImageTexture(0, context.texPos->id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        computeProgram.uniform4f("uLightPos", lightPos(0), lightPos(1), lightPos(2), lightPos(3));
        computeProgram.uniform2i("uTexSize", context.frameWidth, context.frameHeight);
        computeProgram.uniformMatrix4fv("V", 1, vt.mat);
        computeProgram.uniform4f("Vt", vt.pos(0), vt.pos(1), vt.pos(2), vt.pos(3));
        // Perform the actual computation
        svComputer.compute(MCompBuffer, MtCompBuffer);
        
        /// Deferred rendering
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        context.setViewport(context.frameWidth, context.frameHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        quadProgram.use();
        quadProgram.uniform1f("uLightIntensity", lightIntensity);
        quadProgram.uniform1f("uLightRadius", lightRadius);
        quadProgram.uniform4f("uLightPos", lightPos(0), lightPos(1), lightPos(2), lightPos(3));
        quadProgram.uniform2i("uTexSize", context.frameWidth, context.frameHeight);
        context.memoryBarrier(Empty::gl::MemoryBarrierType::ShaderStorage);
        quadRC.render();
        
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
                ImGui::SliderFloat("Rotation divisor X", &camera.rotationDivisorX, 1, (float)context.frameWidth);
                ImGui::SliderFloat("Rotation divisor Y", &camera.rotationDivisorY, 1, (float)context.frameHeight);
                ImGui::SliderFloat("XW+ZW rotation speed", &camera.xwzwSpeed, 0.1f, (float)M_PI * 2.f);
                ImGui::TreePop();
            }
        ImGui::End();
        
        ImGui::Begin("Debug info", NULL, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Rendering %d tetrahedra at %.1f FPS", tetrahedra, ImGui::GetIO().Framerate);
            ImGui::Text("Camera position : %lf, %lf, %lf, %lf",
                camera.pos(0), camera.pos(1), camera.pos(2), camera.pos(3));
            ImGui::Text("Camera rotation : %lf, %lf, %lf", camera._xz, camera._yz, camera._xwzw);
        ImGui::End();
                
        context.swap();

        if(!context.freezeCamera)
            camera.update(dt);
    }
    
    trace("Exiting drawing loop");
    
    // Cleanup
    context.terminate();
    
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
