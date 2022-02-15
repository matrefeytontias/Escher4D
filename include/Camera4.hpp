#ifndef INC_CAMERA4
#define INC_CAMERA4

#include <Empty/math/mat.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Transform4.hpp"

struct Camera4 : private Transform4
{
    Camera4(GLFWwindow *window) : Transform4(), _window(window)
    {
        glfwGetCursorPos(_window, &_prevMouseX, &_prevMouseY);
    }
    /**
     * Computes the view transform of the scene.
     */
    Transform4 computeViewTransform() const
    {
        // TODO
        // math::mat4 m = mat.inverse();
        math::mat4 m = mat;
        // TODO
        // return Transform4(m, -(m * pos));
        return Transform4(m, pos);
    }
    /**
     * Processes input for camera movement.
     * @param   dt  time step for this frame
     */
    void update(float dt)
    {
        // Movement
        math::vec4 dr(glfwGetKey(_window, GLFW_KEY_D) - glfwGetKey(_window, GLFW_KEY_A),
            0, glfwGetKey(_window, GLFW_KEY_S) - glfwGetKey(_window, GLFW_KEY_W), 0);
        dr = math::normalize(dr);
        dr *= speed * dt;
        
        dr = mat * dr;
        dr(1) = 0; // let's not fly off
        pos += dr;
        
        // Field of view rotation
        double mouseX, mouseY;
        glfwGetCursorPos(_window, &mouseX, &mouseY);
        _xz += (mouseX - _prevMouseX) / rotationDivisorX;
        _yz += (mouseY - _prevMouseY) / rotationDivisorY;
        // Cap _yz rotation at head and feet
        _yz = clamp(_yz, -(float)M_PI / 2 + 0.01f, (float)M_PI / 2 - 0.01f);
        
        math::vec4 dir = math::vec4(-sin(_xz) * cos(_yz), sin(_yz), cos(_xz) * cos(_yz), 0.f);
        lookAt(dir, math::vec4(0, 1, 0, 0), math::vec4(0, 0, 0, 1));
        _prevMouseX = mouseX; _prevMouseY = mouseY;
        
        // XW + ZW rotation
        _xwzw += xwzwSpeed * dt * (glfwGetKey(_window, GLFW_KEY_E) - glfwGetKey(_window, GLFW_KEY_Q));
        rotate(XW, _xwzw);
        rotate(ZW, _xwzw);
    }
    
    /**
     * Ratio between the amount of camera rotation and the amount of pixels traveled
     * by the mouse on the X axis. Unit is px/rad.
     */
    float rotationDivisorX = 400.f;
    /**
     * Ratio between the amount of camera rotation and the amount of pixels traveled
     * by the mouse on the Y axis. Unit is px/rad.
     */
    float rotationDivisorY = 400.f;
    /**
     * Movement speed in unit/s.
     */
    float speed = 5.f;
    /**
     * Rotation speed on the XW+ZW planes in rad/s.
     */
    float xwzwSpeed = 1.;
    
private:
    friend int _main(int argc, char *argv[]);
    // window that the camera is attached to
    GLFWwindow *_window;
    // Mouse position on the previous frame
    double _prevMouseX, _prevMouseY;
    float _xz = 0, _yz = 0, _xwzw = 0;
};

#endif
