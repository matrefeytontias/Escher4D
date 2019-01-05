#ifndef INC_OBJECT4
#define INC_OBJECT4

#include "Camera4.hpp"
#include "Model4RenderContext.hpp"
#include "Transform4.hpp"

/**
 * Represents any object in a 4D space. Used to help chain transforms and create
 * object hierarchies.
 */
struct Object4 : public Transform4
{
    Object4() : Transform4() { }
    Object4(const Object4 &obj) : Transform4(obj.mat, obj.pos)
    {
        _rc = obj._rc;
        _children.assign(obj._children.begin(), obj._children.end());
        color = obj.color;
    }
    Object4(Model4RenderContext &rc) : Transform4() { _rc = &rc; }
    
    /**
     * Sets the rendering context for this object. Use `nullptr` to mark this
     * object as not to be rendered.
     */
    Object4 &setRenderContext(Model4RenderContext *rc)
    {
        _rc = rc;
        return *this;
    }
    /**
     * Adds an object as a children to this object.
     */
    Object4 &addChild(Object4 *c)
    {
        _children.push_back(c);
        return *this;
    }
    
    /**
     * Renders this object and all its children.
     */
    void render(const Camera4 &camera)
    {
        render(camera.computeViewTransform());
    }
    
    Vector4f color;
    
protected:
    void render(const Transform4 &vt)
    {
        Transform4 mv = chain(vt);
        // Render self
        if(_rc)
        {
            _rc->_program.use();
            _rc->_program.uniformMatrix4fv("MV", 1, &mv.mat.data()[0]);
            Matrix4f tinvmv = mv.mat.inverse().transpose();
            _rc->_program.uniformMatrix4fv("tinvMV", 1, &tinvmv.data()[0]);
            _rc->_program.uniform4f("MVt", mv.pos(0), mv.pos(1), mv.pos(2), mv.pos(3));
            _rc->_program.uniform4f("uColor", color(0), color(1), color(2), color(3));
            _rc->render();
        }
        
        // Render children
        for(Object4 *obj : _children)
            obj->render(mv);
    }
    Model4RenderContext *_rc = nullptr;
    std::vector<Object4*> _children;
};

#endif
