#ifndef INC_MODEL4_RENDER_CONTEXT
#define INC_MODEL4_RENDER_CONTEXT

#include "Geometry4.hpp"
#include "RenderContext.hpp"
#include "ShaderProgram.hpp"
#include "Transform4.hpp"

class Model4RenderContext : public RenderContext
{
public:
    Model4RenderContext(Geometry4 &geom, ShaderProgram &program);
    virtual ~Model4RenderContext() { }
    virtual void render() override;
    
    Geometry4 &geometry;
};

#endif
