#version 430 core
#extension GL_NV_shader_thread_group : require

layout(binding = 0, rgba8) restrict uniform image2D texColor;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

struct Plane
{
    vec4 n;
    float c;
};

// Intersection test between a plane and an AABB.
// Returns +1 if the box is all the way above the plane, 0 if it
// intersects the plane or -1 if it is all the way under the plane.
float testPlaneAABB(in Plane p, in vec4 bmin, in vec4 bmax)
{
    vec4 center = (bmax + bmin) / 2;
    float h = dot(center, p.n) - p.c;
    return sign(h) * float(abs(h) > dot(bmax - center, abs(p.n)));
}

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    imageStore(texColor, coord, 1. - imageLoad(texColor, coord));
}
