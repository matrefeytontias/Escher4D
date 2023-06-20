#ifndef INC_FSQUAD_RENDER_CONTEXT
#define INC_FSQUAD_RENDER_CONTEXT

#include "Escher4D/Context.h"
#include "Escher4D/RenderContext.hpp"
#include "Escher4D/utils.hpp"

#include <Empty/gl/Buffer.h>
#include <Empty/gl/ShaderProgram.hpp>
#include <Empty/gl/VertexArray.h>

/**
 * Full-screen quad render context. Used to render a full-screen quad to the
 * screen with an arbitrary shader, useful for things like deferred rendering.
 */
class FSQuadRenderContext : public RenderContext
{
public:
    FSQuadRenderContext(Empty::gl::ShaderProgram &p) : RenderContext(p)
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

        Empty::gl::VertexStructure vs;

        vs.add("aPosition", Empty::gl::VertexAttribType::Float, 2);
        _program.locateAttributes(vs);
        _vao.attachVertexBuffer(_vbo, vs);
        _vao.attachElementBuffer(_ebo);
    }
    
    virtual void render() override
    {
        Context& context = Context::get();
        context.bind(_vao);
        context.drawElements(Empty::gl::PrimitiveType::Triangles, Empty::gl::ElementType::Int, 0, 6);
    }
protected:
    Empty::gl::Buffer _vbo, _ebo;
    Empty::gl::VertexArray _vao;
};

#endif
