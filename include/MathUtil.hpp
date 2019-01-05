#ifndef INC_MATH_UTIL
#define INC_MATH_UTIL

#include <iterator>
#include <vector>

#include <Eigen/Eigen>

using namespace Eigen;

namespace MathUtil
{

/**
 * 4D cross product.
 */
Vector4f cross4(const Vector4f &v1, const Vector4f &v2, const Vector4f &v3);
/**
 * Returns the index of the nearest point in an array to a point.
 */
std::vector<Vector4f>::iterator nearestPoint(const Vector4f &v, std::vector<Vector4f> &vs);

};

#endif
