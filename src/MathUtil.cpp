#include "MathUtil.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

#include <Eigen/Eigen>

using namespace Eigen;

Vector4f MathUtil::cross4(const Vector4f &v1, const Vector4f &v2, const Vector4f &v3)
{
    static auto xyz = [](const Vector4f &v) { return Vector3f(v(0), v(1), v(2)); };
    static auto xyw = [](const Vector4f &v) { return Vector3f(v(0), v(1), v(3)); };
    static auto xzw = [](const Vector4f &v) { return Vector3f(v(0), v(2), v(3)); };
    static auto yzw = [](const Vector4f &v) { return Vector3f(v(1), v(2), v(3)); };
    
    Vector4f v;
    v(0) = yzw(v3).dot(yzw(v1).cross(yzw(v2)));
    v(1) = -xzw(v3).dot(xzw(v1).cross(xzw(v2)));
    v(2) = xyw(v3).dot(xyw(v1).cross(xyw(v2)));
    v(3) = -xyz(v3).dot(xyz(v1).cross(xyz(v2)));
    return v;
}

std::vector<Vector4f>::iterator MathUtil::nearestPoint(const Vector4f &v, std::vector<Vector4f> &vs)
{
    return std::min_element(vs.begin(), vs.end(), [&v](Vector4f &v1, Vector4f &v2)
    { return (v - v1).squaredNorm() < (v - v2).squaredNorm(); });
}
