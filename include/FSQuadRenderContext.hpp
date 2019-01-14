#ifndef INC_FSQUAD_RENDER_CONTEXT
#define INC_FSQUAD_RENDER_CONTEXT

#include "RenderContext.hpp"
#include "utils.hpp"

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
        glGenBuffers(2, _bo);
        glBindBuffer(GL_ARRAY_BUFFER, _bo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadV), quadV, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _bo[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadT), quadT, GL_STATIC_DRAW);
        _program.use();
        _program.vertexAttribPointer("aPosition", 2, GL_FLOAT, 0, 0);
    }
    
    ~FSQuadRenderContext()
    {
        glDeleteBuffers(2, _bo);
    }
    
    virtual void render() override
    {
        glBindBuffer(GL_ARRAY_BUFFER, _bo[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _bo[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
    }
protected:
    GLuint _bo[2];
};

#endif
