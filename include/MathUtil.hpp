#ifndef INC_MATH_UTIL
#define INC_MATH_UTIL

#include <Eigen/Eigen>

using namespace Eigen;

namespace MathUtil
{

/**
 * 4D cross product.
 */
Vector4f cross4(const Vector4f &v1, const Vector4f &v2, const Vector4f &v3);

};

#endif
