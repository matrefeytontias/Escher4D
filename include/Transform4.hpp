#ifndef INC_TRANSFORM4
#define INC_TRANSFORM4

#include <assert.h>
#include <vector>

#include <Eigen/Eigen>

#include "MathUtil.hpp"
#include "Rotor4.hpp"
#include "utils.hpp"

using namespace Eigen;

enum Planes4
{
    XY = 0x01,
    XZ = 0x02,
    XW = 0x03,
    YZ = 0x12,
    YW = 0x13,
    ZW = 0x23
};

struct Transform4
{
    Transform4() : parent(nullptr) { mat = Matrix4f::Identity(); pos = Vector4f::Zero(); }
    Transform4(const Matrix4f &m, const Vector4f &t) : mat(m), pos(t), parent(nullptr) { }
    
    /**
     * Applies this transform to a vector.
     */
    Vector4f apply(const Vector4f &v) const
    {
        Vector4f r = mat * v + pos;
        return parent ? parent->apply(r) : r;
    }
    
    /**
     * Chains a transform to this one, resulting in a new transform that applies
     * this transform, then the other transform.
     */
    Transform4 chain(const Transform4 &b) const
    {
        Transform4 t;
        t.mat = b.mat * mat;
        t.pos = b.mat * pos + b.pos;
        t.parent = b.parent;
        return t;
    }
    
    /**
     * Appends a scaling to the current transform.
     * @param
     */
    Transform4 &scale(const Vector4f &factor)
    {
        Matrix4f s = Matrix4f::Identity();
        for(int k = 0; k < 4; ++k)
            s(k, k) = factor(k);
        mat = s * mat;
        return *this;
    }
    
    /**
     * Appends a rotation to the current transform.
     * @param   p       plane of rotation
     * @param   angle   angle of rotation
     */
    Transform4 &rotate(Planes4 p, float angle)
    {
        Matrix4f r = Matrix4f::Identity();
        int i = p >> 4, j = p & 0xf;
        float c = cos(angle), s = sin(angle);
        r(i, i) = r(j, j) = c;
        r(i, j) = s;
        r(j, i) = -s;
        mat = r * mat;
        return *this;
    }
    
    /**
     * Makes the current transform into a "look at" operation, ie aligns the Z
     * axis with a given direction using the given two extra axis to stay in a
     * fixed 4D hyperplane. Leaves the translational part untouched.
     * @param   at      the direction to look at
     * @param   up      the up vector defining the hyperplane to look in. Should be normalized
     * @param   duth    the duth vector (w) normal to the hyperplane to look in. Should be normalized
     */
    Transform4 &lookAt(const Vector4f &at, const Vector4f &up, const Vector4f &duth)
    {
        Matrix4f r;
        r.block<4, 1>(0, 2) = at;
        r.block<4, 1>(0, 0) = MathUtil::cross4(up, at, duth).normalized();
        r.block<4, 1>(0, 1) = MathUtil::cross4(at, duth, r.block<4, 1>(0, 0)).normalized();
        r.block<4, 1>(0, 3) = MathUtil::cross4(r.block<4, 1>(0, 0), r.block<4, 1>(0, 1), at).normalized();
        
        mat = r;
        return *this;
    }
    
    /**
     * Linear component of the transform.
     */
    Matrix4f mat;
    // Rotor4 mat;
    /**
     * Translation component of the transform.
     */
    Vector4f pos;
    /**
     * Optional parent to this transform.
     */
    Transform4 *parent;
};

#endif
