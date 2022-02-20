#ifndef INC_OBJECT4
#define INC_OBJECT4

#include <functional>
#include <memory>
#include <cstdarg>

#include "Camera4.hpp"
#include "Model4RenderContext.hpp"
#include "Transform4.hpp"

/**
 * Represents any object in a 4D space. Used to help chain transforms and create
 * object hierarchies.
 */
struct Object4 : public Transform4
{
    static Object4 scene;
    
    typedef std::shared_ptr<Object4> Object4Ptr;
    
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
     * Returns an immutable pointer to the render context of this object, if any.
     */
    const Model4RenderContext *getRenderContext() const
    {
        return _rc;
    }
    
    /**
     * Return references to children object.
     */
    Object4 &operator[](unsigned int k)
    {
        return *_children[k];
    }
    
    /**
     * Creates a new object as a child to this object and returns a reference to it.
     */
    Object4 &addChild()
    {
        _children.push_back(Object4Ptr(new Object4));
        return *_children.back();
    }
    /**
     * Adds an object as a child to this object and returns a reference to it.
     */
    Object4 &addChild(Object4 *c)
    {
        _children.push_back(Object4Ptr(c));
        return *_children.back();
    }
    /**
     * Adds the copy of an object as a child to this object and returns a reference to it.
     */
    Object4 &addChild(const Object4 &obj)
    {
        _children.push_back(Object4Ptr(new Object4(obj)));
        return *_children.back();
    }
    /**
     * Creates an object as a child to this object given a render context and returns a reference to it.
     */
    Object4 &addChild(Model4RenderContext &rc)
    {
        _children.push_back(Object4Ptr(new Object4(rc)));
        return *_children.back();
    }
    
    /**
     * Removes children using their index.
     */
    void removeChild(unsigned int k)
    {
        _children.erase(_children.begin() + k);
    }
    
    /**
     * Removes children using their index.
     */
    template <typename ... T>
    void removeChild(unsigned int k, T ... args)
    {
        removeChild(k);
        removeChild((args - 1)...);
    }
    
    /**
     * Executes a function on an object and all its children recursively.
     */
    void visit(const std::function<void(const Object4&)> &f) const
    {
        f(*this);
        for(const Object4Ptr &c : _children)
            c->visit(f);
    }
    /**
     * Executes a function on an object and all its children recursively while
     * propagating an argument.
     */
    template <typename T>
    void visit(const std::function<T(const Object4&, T&)> &f, T &init) const
    {
        T ret = f(*this, init);
        for(const Object4Ptr &c : _children)
            c->visit(f, ret);
    }
    
    /**
     * Returns a shallow copy of this object, that is, a new object that has the
     * same children as this one.
     */
    Object4 shallowCopy()
    {
        return Object4(*this);
    }
    
    /**
     * Returns a deep copy of this object, that is, a new object where all the
     * children are deep copies of the original's children.
     */
    Object4 deepCopy()
    {
        Object4 result(*this);
        result._children.clear();
        for(Object4Ptr &obj : _children)
            result.addChild(obj->deepCopy());
        return result;
    }
    
    /**
     * Renders this object and all its children.
     */
    void render(const Camera4 &camera)
    {
        render(camera.computeViewTransform());
    }
    
    math::vec4 color;
    
    bool castShadows = true;
    bool insideOut = false;
    
protected:
    Object4() : Transform4() { }
    Object4(const Object4 &obj) : Transform4(obj.mat, obj.pos)
    {
        _rc = obj._rc;
        _children.assign(obj._children.begin(), obj._children.end());
        color = obj.color;
    }
    Object4(Model4RenderContext &rc) : Transform4() { _rc = &rc; }
    void render(const Transform4 &vt)
    {
        Transform4 mv = chain(vt);
        // Render self
        if(_rc)
        {
            _rc->_program.use();
            _rc->_program.uniformMatrix4fv("MV", 1, mv.mat);
            math::mat4 tinvmv = math::transpose(math::inverse(mv.mat));
            _rc->_program.uniformMatrix4fv("tinvMV", 1, tinvmv);
            _rc->_program.uniform4f("MVt", mv.pos(0), mv.pos(1), mv.pos(2), mv.pos(3));
            _rc->_program.uniform4f("uColor", color(0), color(1), color(2), color(3));
            _rc->_program.uniform1i("uInsideOut", insideOut);
            _rc->render();
        }
        
        // Render children
        for(Object4Ptr &obj : _children)
            obj->render(mv);
    }
    Model4RenderContext *_rc = nullptr;
    std::vector<Object4Ptr> _children;
};

#endif
