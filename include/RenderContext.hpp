#ifndef INC_RENDER_CONTEXT
#define INC_RENDER_CONTEXT

#include <Empty/gl/ShaderProgram.hpp>

class RenderContext
{
public:
    RenderContext(Empty::gl::ShaderProgram &program) : _program(program) { }
    virtual void render() = 0;
protected:
    friend struct Object4;
    Empty::gl::ShaderProgram &_program;
};

#endif
