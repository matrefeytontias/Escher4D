#ifndef INC_GEOMETRY4
#define INC_GEOMETRY4

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include <Eigen/Eigen>

#include "MathUtil.hpp"
#include "ShaderProgram.hpp"
#include "utils.hpp"

using namespace Eigen;
using namespace std;

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
    static Geometry4 from3D(const vector<Vector3f> &v3, const vector<unsigned int> &tetras)
    {
        Geometry4 g;
        
        for(const Vector3f &v : v3)
            g.vertices.push_back(Vector4f(v(0), v(1), v(2), 0));
        g.cells.assign(tetras.begin(), tetras.end());
        
        return g;
    }
    
    /**
     * Constructs 4D geometry by extruding 3D geometry along the W axis by a set
     * amount. This does not compute normal vectors automatically !
     * @param   v3      array of 3D vertices
     * @param   tris    array of triangle indices
     * @param   tetras  array of tetrahedron indices
     * @param   duth    amount of extrusion along the W axis
     */
    static Geometry4 from3D(const vector<Vector3f> &v3, const vector<unsigned int> &tris,
        const vector<unsigned int> &tetras, float duth)
    {
        Geometry4 g;
        int base = v3.size();
        float d = duth / 2;
        
        for(const Vector3f &v : v3)
        {
            Vector4f v4(v(0), v(1), v(2), -d);
            g.vertices.push_back(v4);
        }
        
        g.cells.insert(g.cells.begin(), tetras.begin(), tetras.end());
        for(int t : tetras)
            g.cells.push_back(t + base);
        
        for(const Vector3f &v : v3)
        {
            Vector4f v4(v(0), v(1), v(2), d);
            g.vertices.push_back(v4);
        }
        
        for(unsigned int k = 0; k < tris.size(); k += 3)
        {
            int a = tris[k], b = tris[k + 1], c = tris[k + 2],
                d = a + base, e = b + base, f = c + base;
            g.pushCell(a, c, e, d);
            g.pushCell(b, e, c, a);
            g.pushCell(c, e, f, d);
        }
        
        return g;
    }
    
    /**
     * Recomputes the geometry's normal vectors, using solid angle weighting if
     * the mesh is indexed.
     * @param   inwards     whether the normal vectors should face inwards or outwards
     */
    void recomputeNormals(bool inwards = false)
    {
        static Vector4f zero = Vector4f::Zero();
        normals.clear();
        
        // If the mesh is indexed, use solid angle weighting
        if(cells.size() > 0)
        {
            normals.resize(vertices.size(), Vector4f::Zero());
            for(unsigned int c = 0; c < cells.size(); c += 4)
            {
                unsigned int *cell = &cells[c];
                Vector4f cellN = MathUtil::cross4(vertices[cell[1]] - vertices[cell[0]], vertices[cell[2]] - vertices[cell[0]],
                    vertices[cell[3]] - vertices[cell[0]]);
                float paraVolume = cellN.norm();
                auto sdist = [&](int i, int j) { return (vertices[cell[i % 4]] - vertices[cell[j % 4]]).squaredNorm(); };
                
                // Skeleton checking for normal vector orientation
                Vector4f &checker = zero;
                if(skeleton.size() > 0)
                    checker = *MathUtil::nearestPoint(vertices[cell[0]], skeleton);
                if((cellN.dot(vertices[cell[0]] - checker) < 0) != inwards)
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
            for(unsigned int i = 0; i < vertices.size(); i += 4)
            {
                Vector4f n = MathUtil::cross4(vertices[i + 1] - vertices[i], vertices[i + 2] - vertices[i],
                    vertices[i + 3] - vertices[i]);
                
                // Skeleton checking for normal vector orientation
                Vector4f &checker = zero;
                if(skeleton.size() > 0)
                    checker = *MathUtil::nearestPoint(vertices[i], skeleton);
                if((n.dot(vertices[i] - checker) < 0) != inwards)
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
        
        for(Vector4f &v : normals)
            v.normalize();
    }
    
    /**
     * Computes the barycenter of the geometry.
     */
    Vector4f barycenter() const
    {
        Vector4f z = Vector4f::Zero();
        return accumulate(vertices.begin(), vertices.end(), z) / vertices.size();
    }
    
    /**
     * Computes the bounding box of the geometry.
     */
    void boundingBox(Vector4f &min, Vector4f &max)
    {
        min = max = vertices[0];
        for(Vector4f &v : vertices)
        {
            min = v.array().min(min.array()).matrix();
            max = v.array().max(max.array()).matrix();
        }
    }
    
    /**
     * Unindexes the mesh. Gets rid of the cells array and duplicates vertices
     * where necessary.
     */
    void unindex()
    {
        vector<Vector4f> newv;
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
        unsigned int v = vertices.size() * sizeof(Vector4f),
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
        int offset = reinterpret_cast<char*>(&vertices[0](0)) - reinterpret_cast<char*>(&vertices[0]);
        program.vertexAttribPointer("aPosition", 4, GL_FLOAT, sizeof(Vector4f), offset);
        program.vertexAttribPointer("aNormal", 4, GL_FLOAT, sizeof(Vector4f),
            vertices.size() * sizeof(Vector4f) + offset);
    }
    
    /**
     * Tells whether the geometry is indexed or has vertex duplication.
     */
    bool isIndexed() { return cells.size() > 0; }
    
    /**
     * Vertices of the geomtry.
     */
    vector<Vector4f> vertices;
    /**
     * Optional skeleton used for normal vector orientation.
     */
    vector<Vector4f> skeleton;
    /**
     * Cell indices. Cells are tetrahedra living in 4-space.
     */
    vector<unsigned int> cells;
    /**
     * Normal vectors at every vertex. This is not automatically recomputed !
     */
    vector<Vector4f> normals;
private:
    GLuint _vbos[ARRAY_BUFFERS];
};

#endif
