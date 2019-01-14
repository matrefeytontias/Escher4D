#include "Model4RenderContext.hpp"

#include <vector>

#include "ShaderProgram.hpp"

Model4RenderContext::Model4RenderContext(Geometry4 &geom, ShaderProgram &program)
    : RenderContext(program), geometry(geom) { }

void Model4RenderContext::render()
{
    geometry.exposeGPU(_program);
    if(geometry.isIndexed())
        glDrawElements(GL_LINES_ADJACENCY, geometry.cells.size(), GL_UNSIGNED_INT, NULL);
    else
        glDrawArrays(GL_LINES_ADJACENCY, 0, geometry.vertices.size());
}
