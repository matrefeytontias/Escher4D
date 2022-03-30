#ifndef INC_GEOMETRY4
#define INC_GEOMETRY4

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include <Empty/gl/Buffer.h>
#include <Empty/gl/ShaderProgram.hpp>
#include <Empty/gl/VertexArray.h>
#include <Empty/gl/VertexStructure.h>
#include <Empty/math/funcs.h>

#include "Context.h"
#include "MathUtil.hpp"
#include "utils.hpp"

/**
 * Handles 4D geometry on the CPU and GPU side.
 */
struct Geometry4
{
    /**
     * Constructs a 4D geometry by promoting 3D vertices to 4D by adding an extra 0.
     * This does not compute normal vectors automatically !
     * @param   v3      array of 3D vertices
     * @param   tetras  array of tetrahedron indices
     */
    void from3D(const std::vector<Empty::math::vec3> &v3, const std::vector<Empty::math::uvec4> &tetras)
    {
        vertices.clear();
        for(const Empty::math::vec3 &v : v3)
            vertices.push_back(Empty::math::vec4(v, 0));
        cells.assign(tetras.begin(), tetras.end());
    }
    
    /**
     * Constructs 4D geometry by extruding 3D geometry along the W axis by a set
     * amount. This does not compute normal vectors automatically !
     * @param   v3      array of 3D vertices
     * @param   tris    array of triangle indices
     * @param   tetras  array of tetrahedron indices
     * @param   duth    amount of extrusion along the W axis
     */
    void from3D(const std::vector<Empty::math::vec3> &v3, const std::vector<Empty::math::uvec3> &tris,
        const std::vector<Empty::math::uvec4> &tetras, float duth)
    {
        vertices.clear();
        int base = static_cast<int>(v3.size());
        float d = duth / 2;
        
        for(const auto &v : v3)
        {
            Empty::math::vec4 v4(v, -d);
            vertices.push_back(v4);
        }
        
        cells.insert(cells.begin(), tetras.begin(), tetras.end());
        for(const auto &t : tetras)
            cells.push_back(t + base);
        
        for(const auto &v : v3)
        {
            Empty::math::vec4 v4(v, d);
            vertices.push_back(v4);
        }
        
        for(const auto& tri : tris)
        {
            int a = tri.x, b = tri.y, c = tri.z,
                d = a + base, e = b + base, f = c + base;
            pushCell(a, c, e, d);
            pushCell(b, e, c, a);
            pushCell(c, e, f, d);
        }
    }
    
    /**
     * Recomputes the geometry's normal vectors, using solid angle weighting if
     * the mesh is indexed.
     * @param   inwards     whether the normal vectors should face inwards or outwards
     */
    void recomputeNormals(bool inwards = false)
    {
        Empty::math::vec4 checker = Empty::math::vec4::zero;
        normals.clear();
        
        // If the mesh is indexed, use solid angle weighting
        if(cells.size() > 0)
        {
            normals.resize(vertices.size(), Empty::math::vec4::zero);
            for(auto &cell : cells)
            {
                auto cellN = MathUtil::cross4(vertices[cell[1]] - vertices[cell[0]], vertices[cell[2]] - vertices[cell[0]],
                    vertices[cell[3]] - vertices[cell[0]]);
                float paraVolume = Empty::math::length(cellN);
                auto sdist = [&](int i, int j) { auto r = (vertices[cell[i % 4]] - vertices[cell[j % 4]]); return Empty::math::dot(r, r); };
                
                // Skeleton checking for normal std::vector orientation
                if(skeleton.size() > 0)
                    checker = *MathUtil::nearestPoint(vertices[cell[0]], skeleton);
                if((Empty::math::dot(cellN, vertices[cell[0]] - checker) < 0) != inwards)
                {
                    std::swap(cell[2], cell[3]);
                    cellN *= -1;
                }
                
                for(unsigned int i = 0; i < 4; ++i)
                {
                    // Use solid angle as weight
                    // cf Relation between edge lengths, dihedral and solid angles in tetrahedra ; Wirth and Dreiding, 2014, theorem 2 and (13)
                    // plus the fact that tetrahedron volume is given as one sixth of the norm of the 4D cross product
                    float eij = sqrt(sdist(i, i + 1)), eik = sqrt(sdist(i, i + 2)), eil = sqrt(sdist(i, i + 3)),
                        sejk = sdist(i + 1, i + 2), sejl = sdist(i + 1, i + 3), sekl = sdist(i + 2, i + 3),
                        Ni = (eij + eik) * (eik + eil) * (eil + eij) - (eij * sekl + eik * sejl + eil * sejk),
                        phi = atan(paraVolume * 2 / Ni);
                    
                    normals[cell[i]] += cellN * phi;
                }
            }
        }
        else
        {
            for(size_t i = 0; i < vertices.size(); i += 4)
            {
                Empty::math::vec4 n = MathUtil::cross4(vertices[i + 1] - vertices[i], vertices[i + 2] - vertices[i],
                    vertices[i + 3] - vertices[i]);
                
                // Skeleton checking for normal std::vector orientation
                if(skeleton.size() > 0)
                    checker = *MathUtil::nearestPoint(vertices[i], skeleton);
                if((Empty::math::dot(n, vertices[i] - checker) < 0) != inwards)
                {
                    std::swap(vertices[i + 2], vertices[i + 3]);
                    n *= -1;
                }
                
                normals.push_back(n);
                normals.push_back(n);
                normals.push_back(n);
                normals.push_back(n);
            }
        }
        
        for(Empty::math::vec4 &v : normals)
            v = Empty::math::normalize(v);
    }
    
    /**
     * Computes the barycenter of the geometry.
     */
    Empty::math::vec4 barycenter() const
    {
        return std::accumulate(vertices.begin(), vertices.end(), Empty::math::vec4::zero) / static_cast<float>(vertices.size());
    }
    
    /**
     * Computes the bounding box of the geometry.
     */
    void boundingBox(Empty::math::vec4 &min, Empty::math::vec4 &max) const
    {
        min = max = vertices[0];
        for(const auto &v : vertices)
        {
            min = Empty::math::min(v, min);
            max = Empty::math::max(v, max);
        }
    }
    
    /**
     * Unindexes the mesh. Gets rid of the cells array and duplicates vertices
     * where necessary.
     */
    void unindex()
    {
        std::vector<Empty::math::vec4> newv;
        for(const auto& cell : cells)
            for(unsigned int i = 0; i < 4; i++)
                newv.push_back(vertices[cell[i]]);
        vertices.assign(newv.begin(), newv.end());
        cells.clear();
    }
    
    /**
     * Convenience function to push 4 integers to the cells array.
     */
    inline void pushCell(unsigned int a, unsigned int b, unsigned int c, unsigned int d)
    {
        cells.push_back({ a, b, c, d });
    }
    
    /**
     * Uploads the geometry to the GPU. Call this every time the geometry is modified
     * to apply the changes.
     */
    void uploadGPU()
    {
        size_t v = vertices.size() * sizeof(vertices[0]),
            e = cells.size() * sizeof(cells[0]);
        _vbo.setStorage(v * 2, Empty::gl::BufferUsage::StaticDraw);
        _vbo.uploadData(0, v, vertices[0]);
        _vbo.uploadData(v, v, normals[0]);
        if(e > 0)
            _ebo.setStorage(e, Empty::gl::BufferUsage::StaticDraw, cells[0]);
    }
    
    /**
     * Exposes the geometry to the GPU through a shader program.
     */
    void exposeGPU(Empty::gl::ShaderProgram &program)
    {
        Empty::gl::VertexStructure vs(vertices.size());
        vs.add("aPosition", Empty::gl::VertexAttribType::Float, 4);
        vs.add("aNormal", Empty::gl::VertexAttribType::Float, 4);
        program.locateAttributes(vs);
        _vao.attachVertexBuffer(_vbo, vs);
        if (isIndexed())
            _vao.attachElementBuffer(_ebo);
        Context::get().bind(_vao);
    }
    
    /**
     * Tells whether the geometry is indexed or has vertex duplication.
     */
    bool isIndexed() const { return cells.size() > 0; }
    
    /**
     * Vertices of the geomtry.
     */
    std::vector<Empty::math::vec4> vertices;
    /**
     * Optional skeleton used for normal std::vector orientation.
     */
    std::vector<Empty::math::vec4> skeleton;
    /**
     * Cell indices. Cells are tetrahedra living in 4-space.
     */
    std::vector<Empty::math::uvec4> cells;
    /**
     * Normal vectors at every vertex. This is not automatically recomputed !
     */
    std::vector<Empty::math::vec4> normals;
private:
    Empty::gl::VertexArray _vao;
    Empty::gl::Buffer _vbo, _ebo;
};

#endif
