#ifndef INC_MODEL4_RENDER_CONTEXT
#define INC_MODEL4_RENDER_CONTEXT

#include "Geometry4.hpp"
#include "RenderContext.hpp"
#include "ShaderProgram.hpp"
#include "Transform4.hpp"

class Model4RenderContext : public RenderContext, public Transform4
{
public:
    Model4RenderContext(Geometry4 &geom, ShaderProgram &program);
    virtual void render() override;
    Vector4f color;
protected:
    Geometry4 _geometry;
};

#endif
