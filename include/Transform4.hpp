#ifndef INC_TRANSFORM4
#define INC_TRANSFORM4

#include <assert.h>
#include <vector>

#include <Empty/math/funcs.h>

#include "MathUtil.hpp"
#include "Rotor4.hpp"
#include "utils.hpp"

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
    Transform4() { mat = math::mat4::Identity(); pos = math::vec4(0, 0, 0, 0); }
    Transform4(const math::mat4 &m, const math::vec4 &t) : mat(m), pos(t) { }
    
    /**
     * Applies this transform to a vector.
     */
    math::vec4 apply(const math::vec4 &v) const
    {
        return mat * v + pos;
    }
    
    /**
     * Chains a transform to this one, resulting in a new transform that applies
     * this transform, then the other transform.
     */
    Transform4 chain(const Transform4 &b) const
    {
        return Transform4(b.mat * mat, b.mat * pos + b.pos);
    }
    
    /**
     * Appends a scaling to the current transform.
     * @param
     */
    Transform4 &scale(const math::vec4 &factor)
    {
        math::mat4 s = math::mat4::Identity();
        for(int k = 0; k < 4; ++k)
            s(k, k) = factor(k);
        mat = s * mat;
        return *this;
    }
    
    Transform4 &scale(float factor)
    {
        mat *= factor;
        return *this;
    }
    
    /**
     * Appends a rotation to the current transform.
     * @param   p       plane of rotation
     * @param   angle   angle of rotation
     */
    Transform4 &rotate(Planes4 p, float angle)
    {
        math::mat4 r = math::mat4::Identity();
        int i = p >> 4, j = p & 0xf;
        float c = cos(angle), s = sin(angle);
        r(i, i) = r(j, j) = c;
        r(i, j) = -s;
        r(j, i) = s;
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
    Transform4 &lookAt(const math::vec4 &at, const math::vec4 &up, const math::vec4 &duth)
    {
        math::mat4 r;
        r.column(2) = at;
        r.column(0) = math::normalize(MathUtil::cross4(up, at, duth));
        r.column(1) = math::normalize(MathUtil::cross4(at, duth, r.column(0)));
        r.column(3) = math::normalize(MathUtil::cross4(r.column(0), r.column(1), at));
        
        mat = r;
        return *this;
    }
    
    /**
     * Linear component of the transform.
     */
    math::mat4 mat;
    // Rotor4 mat;
    /**
     * Translation component of the transform.
     */
    math::vec4 pos;
};

#endif
