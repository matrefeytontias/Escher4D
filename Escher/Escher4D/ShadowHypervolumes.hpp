#ifndef INC_SHADOW_HYPERVOLUMES
#define INC_SHADOW_HYPERVOLUMES

#include <vector>

#include <Empty/gl/Buffer.h>
#include <Empty/gl/ShaderProgram.hpp>
#include <Empty/math/vec.h>
#include <Empty/math/mat.h>
#include <GLFW/glfw3.h>

#include "Escher4D/Context.h"

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
        _aabbProgram.attachFile(Empty::gl::ShaderType::Compute, "shaders/reduction_compute.glsl");
        _aabbProgram.build();
        _computeProgram.attachFile(Empty::gl::ShaderType::Compute, "shaders/test_compute.glsl");
        _computeProgram.build();
    }
    
    /**
     * Re-initializes the state of the shadow volumes computer. Call this when changing
     * screen dimensions, shadow-casting tetrahedra or vertices.
     */
    void reinit(int w, int h, const Empty::gl::TextureInfo &texPos, const std::vector<Empty::math::uvec4> &cells, const std::vector<unsigned int> &objIndices, const std::vector<Empty::math::vec4> &vertices)
    {
        _w = w;
        _h = h;
        _cellsAmount = static_cast<int>(cells.size());
        _cellBuf.setStorage(cells.size() * sizeof(cells[0]), Empty::gl::BufferUsage::StaticDraw, cells[0]);
        _objIDBuf.setStorage(objIndices.size() * sizeof(objIndices[0]), Empty::gl::BufferUsage::StaticDraw, &objIndices[0]);
        _vertexBuf.setStorage(vertices.size() * sizeof(vertices[0]), Empty::gl::BufferUsage::StaticDraw, vertices[0]);
        // AABB hierarchy has 4 * 2 floats per pixel
        _aabbBuf.setStorage(BUFFER_SIZE * 4 * 2 * sizeof(float), Empty::gl::BufferUsage::DynamicCopy);
        // Shadow hierarchy has 1 bit per pixel but OpenGL needs ints, so divide the size by 32
        _shadowBuf.setStorage((BUFFER_SIZE + w * h) * sizeof(int) / 32, Empty::gl::BufferUsage::DynamicCopy);
        _aabbProgram.registerTexture("texPos", texPos);
    }
    
    /**
     * Builds the AABB hierarchy and binds the test program. Call this before
     * exposing uniforms to the test program.
     */
    Empty::gl::ShaderProgram &precompute()
    {
        Context& context = Context::get();

        _aabbProgram.uniform("uTexSize", Empty::math::ivec2(_w, _h));
        context.bind(_aabbBuf, Empty::gl::IndexedBufferTarget::ShaderStorage, 5);
        context.memoryBarrier(Empty::gl::MemoryBarrierType::ShaderStorage);
        context.setShaderProgram(_aabbProgram);
        context.dispatchCompute((_w + 7) / 8, (_h  + 3) / 4, 1);
        
        // Clear shadow hierarchy
        _shadowBuf.clearData<Empty::gl::DataFormat::Red, Empty::gl::DataType::UInt>(Empty::gl::BufferDataFormat::Red32ui, 0);
        
        context.setShaderProgram(_computeProgram);
        return _computeProgram;
    }
    
    /**
     * Builds the shadow hierarchy, which is then retrievable through buffer binding
     * #5 in a shader.
     */
    void compute(std::vector<Empty::math::mat4> &ms, std::vector<Empty::math::vec4> &ts)
    {
        Context& context = Context::get();
        
        _matBuf.setStorage(ms.size() * sizeof(Empty::math::mat4), Empty::gl::BufferUsage::StreamDraw, ms[0]);
        _tBuf.setStorage(ts.size() * sizeof(Empty::math::vec4), Empty::gl::BufferUsage::StreamDraw, ts[0]);
        
        context.bind(_cellBuf, Empty::gl::IndexedBufferTarget::ShaderStorage, 0);
        context.bind(_objIDBuf, Empty::gl::IndexedBufferTarget::ShaderStorage, 1);
        context.bind(_vertexBuf, Empty::gl::IndexedBufferTarget::ShaderStorage, 2);
        context.bind(_matBuf, Empty::gl::IndexedBufferTarget::ShaderStorage, 3);
        context.bind(_tBuf, Empty::gl::IndexedBufferTarget::ShaderStorage, 4);
        context.bind(_aabbBuf, Empty::gl::IndexedBufferTarget::ShaderStorage, 5);
        context.bind(_shadowBuf, Empty::gl::IndexedBufferTarget::ShaderStorage, 6);
        
        context.memoryBarrier(Empty::gl::MemoryBarrierType::ShaderStorage);
        context.setShaderProgram(_computeProgram);
        context.dispatchCompute(_cellsAmount, 1, 1);
    }

private:
    const size_t BUFFER_SIZE = 8 * 4 + 32 * 32 + 256 * 128 + 1024 * 1024;

    // 0 : cells, 1 : object indices, 2 : vertices, 3 : M matrices, 4 : translation
    // part of M matrices, 5 : AABB hierarchy, 6 : shadow hierarchy
    Empty::gl::Buffer _cellBuf, _objIDBuf, _vertexBuf, _matBuf, _tBuf, _aabbBuf, _shadowBuf;
    Empty::gl::ShaderProgram _computeProgram, _aabbProgram;
    int _w = 1, _h = 1;
    int _cellsAmount = 0;
};

#endif
