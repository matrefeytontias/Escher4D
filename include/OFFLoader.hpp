#ifndef INC_OFF_LOADER
#define INC_OFF_LOADER

#include <exception>
#include <string>
#include <vector>

#include <Empty/math/vec.h>

#include "Geometry4.hpp"
#include "utils.hpp"

namespace OFFLoader
{
    typedef std::string string;
    
/**
 * Loads a 3D model from a base file name. Fills arrays with "basename.off",
 * "basename.face" and "basename.ele" for vertices, triangles and tetrahedra
 * respectively.
 * @return  whether or not the operation succeeded
 */
bool loadModel(const string &baseName, std::vector<Empty::math::vec3> &v,
    std::vector<Empty::math::uvec3> &tris, std::vector<Empty::math::uvec4> &tetras)
{
    try
    {
        std::vector<string> offFile = split(getFileContents(baseName + ".off"), "\r\n"),
            faceFile = split(getFileContents(baseName + ".face"), "\r\n"),
            tetraFile = split(getFileContents(baseName + ".ele"), "\r\n");
        
        // Read vertices
        if(offFile[0] != "OFF")
            return false;
        size_t nb = stoi(offFile[1]); // grab the first integer
        v.resize(nb);

        for(size_t i = 0; i < nb; i++)
        {
            std::vector<string> vertex = split(offFile[i + 2], " \t");
            v[i].x = stof(vertex[0]);
            v[i].y = stof(vertex[1]);
            v[i].z = stof(vertex[2]);
        }
        // Read triangles
        nb = stoi(faceFile[0]);
        tris.resize(nb);
        for(size_t i = 0; i < nb; i++)
        {
            std::vector<string> tri = split(faceFile[i + 1], " \t");
            tris[i].x = stoi(tri[1]);
            tris[i].y = stoi(tri[2]);
            tris[i].z = stoi(tri[3]);
        }
        // Read tetrahedra
        nb = stoi(tetraFile[0]);
        tetras.resize(nb);
        for(size_t i = 0; i < nb; i++)
        {
            std::vector<string> tetra = split(tetraFile[i + 1], " \t");
            tetras[i].x = stoi(tetra[1]);
            tetras[i].y = stoi(tetra[2]);
            tetras[i].z = stoi(tetra[3]);
            tetras[i].w = stoi(tetra[4]);
        }
        
        return true;
    }
    catch(std::exception&)
    {
        return false;
    }
}
}

#endif
