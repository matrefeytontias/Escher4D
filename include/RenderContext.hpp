#ifndef INC_RENDER_CONTEXT
#define INC_RENDER_CONTEXT

#include "ShaderProgram.hpp"

class RenderContext
{
public:
    RenderContext(ShaderProgram &program) : _program(program) { }
    virtual void render() = 0;
protected:
    friend class Object4;
    ShaderProgram &_program;
};

#endif
