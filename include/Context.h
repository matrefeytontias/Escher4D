#pragma once

#include <Empty/Context.hpp>
#include <Empty/gl/Framebuffer.h>
#include <Empty/gl/Renderbuffer.h>
#include <Empty/utils/macros.h>
#include <Empty/utils/noncopyable.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "ShaderProgram.hpp"

struct Context : public Empty::Context, Empty::utils::noncopyable
{
    ~Context() override
    {
        terminate();
    }

    static Context& get() { return _instance; }

    bool init(const char* title, int w, int h)
    {
        ASSERT(!_init);
        /// Setup window
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
        {
            TRACE("Couldn't initialize GLFW");
            return false;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        window = glfwCreateWindow(w, h, title, NULL, NULL);
        if (!window)
        {
            TRACE("Couldn't create window");
            glfwTerminate();
            return false;
        }
        glfwMakeContextCurrent(window);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        glfwSwapInterval(0); // no v-sync, live on the edge

        /// Setup ImGui binding
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        ImGui_ImplOpenGL3_Init("#version 450");
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

        /// Setup style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsClassic();

        setViewport(w, h);
        frameWidth = w;
        frameHeight = h;

        /// Create context resources
        gBuffer = std::make_unique<Empty::gl::Framebuffer>();
        dBuffer = std::make_unique<Empty::gl::Renderbuffer>();

        _init = true;
        return true;
    }

    /**
     * Sets up internals (framebuffer and textures) for deferred rendering given
     * framebuffer dimensions.
     */
    void setupDeferred(int w, int h)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer->getInfo());

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

        gBuffer->enableColorAttachments(0, 1, 2);

        dBuffer->setStorage(Empty::gl::RenderbufferFormat::Depth, w, h);
        gBuffer->attachRenderbuffer<Empty::gl::FramebufferAttachment::Depth>(*dBuffer);
    }


    void newFrame() const
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void swap() const override
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    void terminate()
    {
        if (_init)
        {
            ImGui_ImplGlfw_Shutdown();
            ImGui_ImplOpenGL3_Shutdown();
            ImGui::DestroyContext();
            glfwTerminate();
            _init = false;
        }
    }

    int frameWidth, frameHeight;
    GLFWwindow* window;
    bool freezeCamera = false;
    // Deferred rendering buffers and textures
    std::unique_ptr<Empty::gl::Framebuffer> gBuffer;
    std::unique_ptr<Empty::gl::Renderbuffer> dBuffer;
    Texture* texPos, * texNorm, * texColor;

private:
    Context() : Empty::Context(), Empty::utils::noncopyable(), frameWidth(0), frameHeight(0), window(nullptr), _init(false) { }
    bool _init;
    static Context _instance;

    // Callbacks
    static void glfw_error_callback(int error, const char* description)
    {
        TRACE("Error " << error << " : " << description);
    }

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        Context& context = Context::get();
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            context.freezeCamera = !context.freezeCamera;
            glfwSetInputMode(window, GLFW_CURSOR, context.freezeCamera ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        }
    };

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    }

    static void framebufferSizeCallback(GLFWwindow*, int width, int height)
    {
        Context::get().setupDeferred(width, height);
    }
};
