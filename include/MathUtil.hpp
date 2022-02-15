#ifndef INC_MATH_UTIL
#define INC_MATH_UTIL

#include <iterator>
#include <vector>

namespace math
{
	template <typename T> struct _vec4;
	using vec4 = _vec4<float>;
}

namespace MathUtil
{

/**
 * 4D cross product.
 */
math::vec4 cross4(const math::vec4 &v1, const math::vec4 &v2, const math::vec4 &v3);
/**
 * Returns the index of the nearest point in an array to a point.
 */
std::vector<math::vec4>::iterator nearestPoint(const math::vec4 &v, std::vector<math::vec4> &vs);

};

#endif
