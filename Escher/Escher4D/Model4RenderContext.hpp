#ifndef INC_MODEL4_RENDER_CONTEXT
#define INC_MODEL4_RENDER_CONTEXT

#include <Empty/gl/ShaderProgram.hpp>

#include "Escher4D/meshes/Geometry4.hpp"
#include "Escher4D/RenderContext.hpp"
#include "Escher4D/Transform4.hpp"

class Model4RenderContext : public RenderContext
{
public:
    Model4RenderContext(Geometry4 &geom, Empty::gl::ShaderProgram &program);
    virtual ~Model4RenderContext() { }
    virtual void render() override;
    
    Geometry4 &geometry;
};

#endif
