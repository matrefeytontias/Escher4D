#include "MathUtil.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

#include <Empty/math/vec.h>
#include <Empty/math/funcs.h>

using namespace math;

vec4 MathUtil::cross4(const vec4 &v1, const vec4 &v2, const vec4 &v3)
{
    static auto xyz = [](const vec4 &v) { return vec3(v(0), v(1), v(2)); };
    static auto xyw = [](const vec4 &v) { return vec3(v(0), v(1), v(3)); };
    static auto xzw = [](const vec4 &v) { return vec3(v(0), v(2), v(3)); };
    static auto yzw = [](const vec4 &v) { return vec3(v(1), v(2), v(3)); };
    
    vec4 v;
    v(0) = dot(yzw(v3), cross(yzw(v1), yzw(v2)));
    v(1) = -dot(xzw(v3), cross(xzw(v1), xzw(v2)));
    v(2) = dot(xyw(v3),cross(xyw(v1), xyw(v2)));
    v(3) = -dot(xyz(v3), cross(xyz(v1), xyz(v2)));
    return v;
}

std::vector<vec4>::iterator MathUtil::nearestPoint(const vec4 &v, std::vector<vec4> &vs)
{
    return std::min_element(vs.begin(), vs.end(), [&v](vec4& v1, vec4& v2)
                            {
                                auto d1 = v - v1, d2 = v - v2;
                                return dot(d1, d1) < dot(d2, d2);
                            });
}
