#if 0
#ifndef INC_ROTOR4
#define INC_ROTOR4

#include <cmath>

#include <Eigen/Eigen>

using namespace Eigen;

enum
{
    E = 0, XY, XZ, XW, YZ, YW, ZW, XYZW
} Planes4;

/**
 * Represents an arbitrary rotation in 4D space. Formally, this is an element of
 * the even subalgebra of the Clifford Cl(4, 0) algebra. Elements are factors of
 * respectively 1, e1e2, e1e3, e1e4, e2e3, e2e4, e3e4, e1e2e3e4.
 */
struct Rotor4
{
    Rotor4() { _data << 1, 0, 0, 0, 0, 0, 0, 0; }
    float &operator(Planes4 i) { return _data(i); }
    
    /**
     * Constructs the rotor that represents a rotation that would move a to b.
     */
    Rotor4 &rotateTowards(Vector4f &a, Vector4f &b)
    {
        Vector4f c = (a + b) / 2;
        return *this = a, c;
    }
    
    /**
     * Clifford transposition of the rotor, ie reversal of the order of the basis
     * products.
     */
    Rotor4 &reverse()
    {
        _data(XY) *= -1;
        _data(XZ) *= -1;
        _data(XW) *= -1;
        _data(YZ) *= -1;
        _data(YW) *= -1;
        _data(ZW) *= -1;
        return *this;
    }
    
    Rotor4 operator*(Rotor4 &b) const
    {
        Rotor4 c;
        Vector4f &d = _data;
        c(E) = d(E) * b(E) + d(XYZW) * b(XYZW);
        for(unsigned int k = 1; k < 7; ++k)
            c(E) -= d(k) * b(k);
        // Ugh
        c(XY) = d(E) * b(XY) + d(XY) * b(E) - d(XZ) * b(YZ) - d(XW) * b(YW) + d(YZ) * b(XZ)
            + d(YW) * b(XW) - d(ZW) * b(XYZW) - d(XYZW) * b(ZW);
        c(XZ) = d(E) * b(XZ) + d(XY) * b(YZ) + d(XZ) * b(E) - d(XW) * b(ZW) - d(YZ) * b(XY)
            + d(YW) * b(XYZW) + d(ZW) * b(XW) + d(XYZW) * b(YW);
        c(XW) = d(E) * b(XW) + d(XY) * b(YW) + d(XZ) * b(ZW) + d(XW) * b(E) - d(YZ) * b(XYZW)
            - d(YW) * b(XY) - d(ZW) * b(XZ) - d(XYZW) * b(YZ);
        c(YZ) = d(E) * b(YZ) - d(XY) * b(XZ) + d(XZ) * b(XY) - d(XW) * b(XYZW) + d(YZ) * b(E)
            - d(YW) * b(ZW) + d(ZW) * b(YW) - d(XYZW) * b(XW);
        c(YW) = d(E) * b(YW) - d(XY) * b(XW) + d(XZ) * b(XYZW) + d(XW) * b(XY) + d(YZ) * b(ZW)
            + d(YW) * b(E) - d(ZW) * b(YZ) + d(XYZW) * b(XZ);
        c(ZW) = d(E) * b(ZW) - d(XY) * b(XYZW) - d(XZ) * b(XW) + d(XW) * b(XZ) - d(YZ) * b(YW)
            + d(YW) * b(YZ) + d(ZW) * b(E) - d(XYZW) * b(XY);
        c(XYZW) = d(E) * b(XYZW) + d(XY) * b(ZW) - d(XZ) * b(YW) + d(XW) * b(YZ) + d(YZ) * b(XW)
            - d(YW) * b(XZ) + d(ZW) * b(XY) + d(XYZW) * b(E);
        return c;
    }
    
    /**
     * Square root of the Clifford squared norm.
     */
    float norm() const
    {
        return sqrt(sqnorm());
    }
    
    /**
     * Clifford squared norm, ie the scalar part of r.reverse() * r.
     */
    float sqnorm() const
    {
        float r = 0;
        for(unsigned int k = 0; k < 8; ++k)
            r += _data(k) * _data(k);
        return r;
    }
    
    /**
     * Converts this rotor to 4x4 matrix form.
     */
    Matrix4f toMatrix() const
    {
        // TODO
        return Matrix4f::Zero();
    }
    
private:
    Matrix<float, 8, 1> _data;
};

/**
 * Constructs the rotor that represents a rotation on the plane described by a and
 * b by twice the angle between a and b.
 */
Rotor4 operator,(Vector4f &a, Vector4f &b)
{
    Rotor4 r;
    r(0) = a.dot(b);
    r(1) = a(0) * b(1) - a(1) * b(0);
    r(2) = a(0) * b(2) - a(2) * b(0);
    r(3) = a(0) * b(3) - a(3) * b(0);
    r(4) = a(1) * b(2) - a(2) * b(1);
    r(5) = a(1) * b(3) - a(3) * b(1);
    r(6) = a(2) * b(3) - a(3) * b(2);
    return r;
}

#endif
#endif
