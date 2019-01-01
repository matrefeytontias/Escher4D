#ifndef INC_SCENE
#define INC_SCENE

#include <vector>

#include <Eigen/Eigen>

#include "Camera4.hpp"
#include "Model4RenderContext.hpp"

struct ModelGroup : public Transform4
{
    void render(Camera4 &camera)
    {
        auto vt = camera.computeViewTransform();
        for(Model4RenderContext *m : _children)
        {
            m->_program.use();
            auto mv = m->chain(chain(vt));
            m->_program.uniformMatrix4fv("MV", 1, &mv.mat.data()[0]);
            Matrix4f tinvmv = mv.mat.inverse().transpose();
            m->_program.uniformMatrix4fv("tinvMV", 1, &tinvmv.data()[0]);
            m->_program.uniform4f("MVt", mv.pos(0), mv.pos(1), mv.pos(2), mv.pos(3));
            
            m->render();
        }
    }
    
    inline ModelGroup &add(Model4RenderContext &m)
    {
        _children.push_back(&m);
        return *this;
    }
private:
    std::vector<Model4RenderContext*> _children;
};

#endif
