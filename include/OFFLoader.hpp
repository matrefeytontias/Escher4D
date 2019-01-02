#ifndef INC_OFF_LOADER
#define INC_OFF_LOADER

#include <exception>
#include <string>
#include <vector>

#include <Eigen/Eigen>

#include "Geometry4.hpp"
#include "utils.hpp"

using namespace Eigen;
using namespace std;

namespace OFFLoader
{
/**
 * Loads a 3D model from a base file name. Fills arrays with "basename.off",
 * "basename.face" and "basename.ele" for vertices, triangles and tetrahedra
 * respectively.
 * @return  whether or not the operation succeeded
 */
bool loadModel(const std::string &baseName, std::vector<Vector3f> &v,
    std::vector<unsigned int> &tris, std::vector<unsigned int> &tetras)
{
    try
    {
        vector<string> offFile = split(getFileContents(baseName + ".off"), "\r\n"),
            faceFile = split(getFileContents(baseName + ".face"), "\r\n"),
            tetraFile = split(getFileContents(baseName + ".ele"), "\r\n");
        
        // Read vertices
        if(offFile[0] != "OFF")
            return false;
        size_t nb = stoi(offFile[1]); // grab the first integer
        v.resize(nb, Vector3f::Zero());
        for(unsigned int i = 0; i < nb; i++)
        {
            vector<string> vertex = split(offFile[i + 2], " \t");
            v[i](0) = stof(vertex[0]);
            v[i](1) = stof(vertex[1]);
            v[i](2) = stof(vertex[2]);
        }
        // Read triangles
        nb = stoi(faceFile[0]);
        tris.resize(nb * 3);
        for(unsigned int i = 0; i < nb; i++)
        {
            vector<string> tri = split(faceFile[i + 1], " \t");
            tris[i * 3] = stoi(tri[1]);
            tris[i * 3 + 1] = stoi(tri[2]);
            tris[i * 3 + 2] = stoi(tri[3]);
        }
        // Read tetrahedra
        nb = stoi(tetraFile[0]);
        tetras.resize(nb * 4);
        for(unsigned int i = 0; i < nb; i++)
        {
            vector<string> tetra = split(tetraFile[i + 1], " \t");
            tetras[i * 4] = stoi(tetra[1]);
            tetras[i * 4 + 1] = stoi(tetra[2]);
            tetras[i * 4 + 2] = stoi(tetra[3]);
            tetras[i * 4 + 3] = stoi(tetra[4]);
        }
        
        return true;
    }
    catch(std::exception &e)
    {
        return false;
    }
}
}

#endif
