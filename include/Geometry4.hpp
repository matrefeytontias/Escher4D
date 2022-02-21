#ifndef INC_GEOMETRY4
#define INC_GEOMETRY4

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include <Empty/math/funcs.h>

#include "MathUtil.hpp"
#include "ShaderProgram.hpp"
#include "utils.hpp"

enum
{
    VERTEX_ARRAY_BUFFER = 0,
    ELEMENT_ARRAY_BUFFER,
    ARRAY_BUFFERS
};

/**
 * Handles 4D geometry on the CPU and GPU side.
 */
struct Geometry4
{
    Geometry4() { glGenBuffers(ARRAY_BUFFERS, _vbos); }
    ~Geometry4() { glDeleteBuffers(ARRAY_BUFFERS, _vbos); }
    
    /**
     * Constructs a 4D geometry by promoting 3D vertices to 4D by adding an extra 0.
     * This does not compute normal vectors automatically !
     * @param   v3      array of 3D vertices
     * @param   tetras  array of tetrahedron indices
     */
    void from3D(const std::vector<Empty::math::vec3> &v3, const std::vector<unsigned int> &tetras)
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
    void from3D(const std::vector<Empty::math::vec3> &v3, const std::vector<unsigned int> &tris,
        const std::vector<unsigned int> &tetras, float duth)
    {
        vertices.clear();
        int base = static_cast<int>(v3.size());
        float d = duth / 2;
        
        for(const Empty::math::vec3 &v : v3)
        {
            Empty::math::vec4 v4(v(0), v(1), v(2), -d);
            vertices.push_back(v4);
        }
        
        cells.insert(cells.begin(), tetras.begin(), tetras.end());
        for(int t : tetras)
            cells.push_back(t + base);
        
        for(const Empty::math::vec3 &v : v3)
        {
            Empty::math::vec4 v4(v(0), v(1), v(2), d);
            vertices.push_back(v4);
        }
        
        for(size_t k = 0; k < tris.size(); k += 3)
        {
            int a = tris[k], b = tris[k + 1], c = tris[k + 2],
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
        static Empty::math::vec4 zero = Empty::math::vec4(0, 0, 0, 0);
        normals.clear();
        
        // If the mesh is indexed, use solid angle weighting
        if(cells.size() > 0)
        {
            normals.resize(vertices.size(), zero);
            for(unsigned int c = 0; c < cells.size(); c += 4)
            {
                unsigned int *cell = &cells[c];
                Empty::math::vec4 cellN = MathUtil::cross4(vertices[cell[1]] - vertices[cell[0]], vertices[cell[2]] - vertices[cell[0]],
                    vertices[cell[3]] - vertices[cell[0]]);
                float paraVolume = Empty::math::length(cellN);
                auto sdist = [&](int i, int j) { auto r = (vertices[cell[i % 4]] - vertices[cell[j % 4]]); return Empty::math::dot(r, r); };
                
                // Skeleton checking for normal std::vector orientation
                Empty::math::vec4 &checker = zero;
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
                Empty::math::vec4 &checker = zero;
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
        return std::accumulate(vertices.begin(), vertices.end(), Empty::math::vec4(0, 0, 0, 0)) / static_cast<float>(vertices.size());
    }
    
    /**
     * Computes the bounding box of the geometry.
     */
    void boundingBox(Empty::math::vec4 &min, Empty::math::vec4 &max) const
    {
        min = max = vertices[0];
        for(const Empty::math::vec4 &v : vertices)
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
        for(unsigned int i : cells)
            newv.push_back(vertices[i]);
        vertices.assign(newv.begin(), newv.end());
        cells.clear();
    }
    
    /**
     * Convenience function to push 4 integers to the cells array.
     */
    inline void pushCell(int a, int b, int c, int d)
    {
        cells.push_back(a);
        cells.push_back(b);
        cells.push_back(c);
        cells.push_back(d);
    }
    
    /**
     * Uploads the geometry to the GPU. Call this every time the geometry is modified
     * to apply the changes.
     */
    void uploadGPU()
    {
        size_t v = vertices.size() * sizeof(Empty::math::vec4),
            e = cells.size() * sizeof(unsigned int);
        glBindBuffer(GL_ARRAY_BUFFER, _vbos[VERTEX_ARRAY_BUFFER]);
        glBufferData(GL_ARRAY_BUFFER, v * 2, NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, v, &vertices[0]);
        glBufferSubData(GL_ARRAY_BUFFER, v, v, &normals[0]);
        if(e > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbos[ELEMENT_ARRAY_BUFFER]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, e, &cells[0], GL_STATIC_DRAW);
        }
    }
    
    /**
     * Exposes the geometry to the GPU through a shader program.
     */
    void exposeGPU(ShaderProgram &program)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _vbos[VERTEX_ARRAY_BUFFER]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbos[ELEMENT_ARRAY_BUFFER]);
        program.vertexAttribPointer("aPosition", 4, GL_FLOAT, sizeof(Empty::math::vec4), 0);
        program.vertexAttribPointer("aNormal", 4, GL_FLOAT, sizeof(Empty::math::vec4),
            vertices.size() * sizeof(Empty::math::vec4));
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
    std::vector<unsigned int> cells;
    /**
     * Normal vectors at every vertex. This is not automatically recomputed !
     */
    std::vector<Empty::math::vec4> normals;
private:
    GLuint _vbos[ARRAY_BUFFERS];
};

#endif
