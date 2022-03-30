#ifndef INC_FSQUAD_RENDER_CONTEXT
#define INC_FSQUAD_RENDER_CONTEXT

#include "Context.h"
#include "RenderContext.hpp"
#include "utils.hpp"

#include <Empty/gl/Buffer.h>

/**
 * Full-screen quad render context. Used to render a full-screen quad to the
 * screen with an arbitrary shader, useful for things like deferred rendering.
 */
class FSQuadRenderContext : public RenderContext
{
public:
    FSQuadRenderContext(ShaderProgram &p) : RenderContext(p)
    {
        const static GLfloat quadV[] = {
            -1, -1,
            1, -1,
            1, 1,
            -1, 1
        };
        const static GLuint quadT[] = {
            0, 1, 2,
            0, 2, 3
        };
        _vbo.setStorage(sizeof(quadV), Empty::gl::BufferUsage::StaticDraw, quadV);
        _ebo.setStorage(sizeof(quadT), Empty::gl::BufferUsage::StaticDraw, quadT);
        
        _program.use();
        glBindBuffer(GL_ARRAY_BUFFER, _vbo.getInfo());
        _program.vertexAttribPointer("aPosition", 2, GL_FLOAT, 0, 0);
    }
    
    virtual void render() override
    {
        Context& context = Context::get();
        context.bind(_vbo, Empty::gl::BufferTarget::Array);
        context.bind(_ebo, Empty::gl::BufferTarget::ElementArray);
        context.drawElements(Empty::gl::PrimitiveType::Triangles, Empty::gl::ElementType::Int, 0, 6);
    }
protected:
    Empty::gl::Buffer _vbo, _ebo;
};

#endif
