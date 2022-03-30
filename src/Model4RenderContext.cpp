#include "Model4RenderContext.hpp"

#include <vector>

#include "Context.h"
#include "ShaderProgram.hpp"

Model4RenderContext::Model4RenderContext(Geometry4 &geom, ShaderProgram &program)
    : RenderContext(program), geometry(geom) { }

void Model4RenderContext::render()
{
    Context& context = Context::get();
    geometry.exposeGPU(_program);
    if (geometry.isIndexed())
        context.drawElements(Empty::gl::PrimitiveType::LinesAdjacency, Empty::gl::ElementType::Int, 0, static_cast<int>(geometry.cells.size() * 4));
    else
        context.drawArrays(Empty::gl::PrimitiveType::LinesAdjacency, 0, static_cast<int>(geometry.vertices.size()));
}
