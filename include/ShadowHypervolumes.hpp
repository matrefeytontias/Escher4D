#ifndef INC_SHADOW_HYPERVOLUMES
#define INC_SHADOW_HYPERVOLUMES

#include <vector>

#include <Empty/math/vec.h>
#include <Empty/math/mat.h>
#include <GLFW/glfw3.h>

#include "ShaderProgram.hpp"

/**
 * Shadow hypervolumes computer. Based off of "An Efficient Alias-free Shadow
 * Algorithm for Opaque and Transparent Objectsusing per-triangle Shadow Volumes",
 * Sintorn et al. (2011) ; generalized for 4D geometry.
 *
 * Using data from G-buffer textures, the class generates a shadow hierarchy that
 * can be sampled in a deferred pipeline fragment shader to implement 4D-accurate,
 * pixel-perfect shadows. See `shaders/reduction_compute.glsl` (precomputing),
 * `shaders/test_compute.glsl` (intersection test) and `shaders/deferred_frag.glsl`.
 */
class ShadowHypervolumes
{
public:
    ShadowHypervolumes()
    {
        glGenBuffers(7, _buffers);
        _aabbProgram.attach(GL_COMPUTE_SHADER, "shaders/reduction_compute.glsl");
        _computeProgram.attach(GL_COMPUTE_SHADER, "shaders/test_compute.glsl");
    }
    
    /**
     * Re-initializes the state of the shadow volumes computer. Call this when changing
     * screen dimensions, shadow-casting tetrahedra or vertices.
     */
    void reinit(int w, int h, Texture &texPos, std::vector<int> &cells, std::vector<int> &objIndices, std::vector<Empty::math::vec4> &vertices)
    {
        _w = w;
        _h = h;
        _cellsAmount = static_cast<int>(cells.size() / 4);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffers[0]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, cells.size() * sizeof(int), &cells[0], GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffers[1]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, objIndices.size() * sizeof(int), &objIndices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffers[2]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, vertices.size() * sizeof(Empty::math::vec4), &vertices[0](0), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffers[5]);
        // AABB hierarchy has 4 * 2 floats per pixel
        glBufferData(GL_SHADER_STORAGE_BUFFER, BUFFER_SIZE * 4 * 2 * sizeof(float), NULL, GL_DYNAMIC_COPY);
        // Shadow hierarchy has 1 bit per pixel but OpenGL needs ints, so divide the size by 32
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffers[6]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, (BUFFER_SIZE + w * h) * sizeof(int) / 32, NULL, GL_DYNAMIC_COPY);
        _aabbProgram.registerTexture("texPos", texPos);
    }
    
    /**
     * Builds the AABB hierarchy and binds the test program. Call this before
     * exposing uniforms to the test program.
     */
    ShaderProgram &precompute()
    {
        _aabbProgram.use();
        _aabbProgram.uniform2i("uTexSize", _w, _h);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDispatchCompute((_w + 7) / 8, (_h  + 3) / 4, 1);
        
        // Clear shadow hierarchy
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffers[6]);
        {
            unsigned int bleh = 0;
            glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &bleh);
        }
        
        _computeProgram.use();
        return _computeProgram;
    }
    
    /**
     * Builds the shadow hierarchy, which is then retrievable through buffer binding
     * #5 in a shader.
     */
    void compute(std::vector<Empty::math::mat4> &ms, std::vector<Empty::math::vec4> &ts)
    {
        // For good measure
        _computeProgram.use();
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffers[3]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ms.size() * sizeof(Empty::math::mat4), ms[0], GL_STREAM_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffers[4]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ts.size() * sizeof(Empty::math::vec4), &ts[0](0), GL_STREAM_DRAW);
        
        for(unsigned int k = 0; k < 7; k++)
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, k, _buffers[k]);
        
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDispatchCompute(_cellsAmount, 1, 1);
    }
    
    ~ShadowHypervolumes()
    {
        glDeleteBuffers(7, _buffers);
    }
    
private:
    const size_t BUFFER_SIZE = 8 * 4 + 32 * 32 + 256 * 128 + 1024 * 1024;
    // 0 : cells, 1 : object index, 2 : vertices, 3 : M matrices, 4 : translation
    // part of M matrices, 5 : AABB hierarchy, 6 : shadow hierarchy
    GLuint _buffers[7];
    ShaderProgram _computeProgram, _aabbProgram;
    int _w = 1, _h = 1;
    int _cellsAmount = 0;
};

#endif
