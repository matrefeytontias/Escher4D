#include "MathUtil.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

#include <Empty/math/vec.h>
#include <Empty/math/funcs.h>

using namespace Empty::math;

vec4 MathUtil::cross4(const vec4 &v1, const vec4 &v2, const vec4 &v3)
{
    vec4 v;
    v.x = dot(v3.yzw(), cross(v1.yzw(), v2.yzw()));
    v.y = -dot(v3.xzw(), cross(v1.xzw(), v2.xzw()));
    v.z = dot(v3.xyw(),cross(v1.xyw(), v2.xyw()));
    v.w = -dot(v3.xyz(), cross(v1.xyz(), v2.xyz()));
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
