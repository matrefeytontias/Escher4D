#include "Model4RenderContext.hpp"

#include <vector>

#include "ShaderProgram.hpp"

Model4RenderContext::Model4RenderContext(Geometry4 &geom, ShaderProgram &program)
    : RenderContext(program), color(Vector4f::Zero()), _geometry(geom) { }

void Model4RenderContext::render()
{
    _geometry.exposeGPU(_program);
    _program.uniform4f("uColor", color(0), color(1), color(2), color(3));
    if(_geometry.isIndexed())
        glDrawElements(GL_LINES_ADJACENCY, _geometry.cells.size(), GL_UNSIGNED_INT, NULL);
    else
        glDrawArrays(GL_LINES_ADJACENCY, 0, _geometry.vertices.size());
}
