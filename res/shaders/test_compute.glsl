#version 430
#extension GL_NV_gpu_shader5 : require
#extension GL_NV_shader_thread_group : require

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
const uint workGroupSize = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;

// Light position in camera space
uniform vec4 uLightPos;

// Inverse projection matrix
uniform mat4 invP;

layout(std430, binding = 0) buffer cellBuffer
{
    ivec4 cells[];
};
layout(std430, binding = 1) buffer objBuffer
{
    int objIndex[];
};
layout(std430, binding = 2) buffer vertexBuffer
{
    vec4 vertices[];
};
layout(std430, binding = 3) buffer MVBuffer
{
    mat4 MV[];
};
layout(std430, binding = 4) buffer MVtBuffer
{
    vec4 MVt[];
};
layout(std430, binding = 5) buffer depthBuffer
{
    vec2 minMaxDepth[];
};
layout(std430, binding = 6) buffer shadowBuffer
{
    uint shadows[];
};

layout(rgba16f, binding = 0) restrict readonly uniform image2D texPos;
layout(r32ui, binding = 1) restrict uniform uimage2D texShadow;

struct Plane
{
    vec4 n;
    float c;
};

struct ShadowVolume
{
    Plane planes[5];
};

// Intersection test between a hyperplane and an AABB.
// Returns +1 if the box is all the way above the plane, 0 if it
// intersects the plane or -1 if it is all the way under the plane.
float testHyperplaneAABB(in Plane p, in vec4 bmin, in vec4 bmax)
{
    vec4 center = (bmax + bmin) / 2;
    float h = dot(center, p.n) - p.c;
    return sign(h) * float(abs(h) > dot(bmax - center, abs(p.n)));
}

// Tests whether a point is over, inside or under a hyperplane.
float testHyperplanePoint(in Plane p, in vec4 v)
{
    return sign(dot(v, p.n) - p.c);
}

// 4D cross product
vec4 cross4(vec4 a, vec4 b, vec4 c)
{
    vec4 v;
    v.x = dot(c.yzw, cross(a.yzw, b.yzw));
    v.y = -dot(c.xzw, cross(a.xzw, b.xzw));
    v.z = dot(c.xyw, cross(a.xyw, b.xyw));
    v.w = -dot(c.xyz, cross(a.xyz, b.xyz));
    return v;
}

// GLSL has no recursion, so we have to inline all the 5 levels of the depth
// buffer hierarchy. Resolutions are 8x4, 32x32, 256x128, 1024x1024, 8192x4096.
const vec2 bufferTileSizes[5] = vec2[5](1. / vec2(8, 4), 1. / vec2(32, 32), 1. / vec2(256, 128),
    1. / vec2(1024, 1024), 1. / vec2(8192, 4096));

vec2 getClipSpaceTileSize(int level)
{
    const vec2 factor = vec2(8192, 4096) / imageSize(texPos);
    return bufferTileSizes[level] * factor;
}

// Tests a tile against a shadow volume in view space.
// Returns -1, 0 or +1 depending on the tile being inside, saddled on or outside the SV.
float testSV(ShadowVolume sv, int level, ivec2 tile)
{
    vec2 tileSize = getClipSpaceTileSize(level);
    vec4 tileMin, tileMax;
    tileMin.xy = vec2(-1.) + tile * tileSize;
    tileMax.xy = tileMin.xy + tileSize;
    // TODO
    return 1;
}

// Tests a view sample against a shadow volume in view space.
bool testSVsample(ShadowVolume sv, ivec2 tile)
{
    // TODO
    return false;
}

// Sets the shadow buffer bit for the given tile at the given level.
void updateShadowBuffer(int level, ivec2 tile)
{
    // TODO
}

// Processes a subtile of the given parentTile. Which subtile
// is determined by the lane ID.

// Level 4 (final level)
void traversal4(ShadowVolume sv, ivec2 parentTile)
{
    const int level = 4;
    ivec2 tile = parentTile * ivec2(8, 4) + ivec2(gl_ThreadInWarpNV % 8, gl_ThreadInWarpNV / 8);
    
    if(testSVsample(sv, tile))
        updateShadowBuffer(level, tile);
}

// Level 3
void traversal3(ShadowVolume sv, ivec2 parentTile)
{
    const int level = 3;
    ivec2 tile = parentTile * ivec2(4, 8) + ivec2(gl_ThreadInWarpNV % 4, gl_ThreadInWarpNV / 4);
    
    float intersects = testSV(sv, level, tile);
    
    if(intersects > 0.)
        updateShadowBuffer(level, tile);
    else
    {
        uint queue = ballotThreadNV(intersects == 0.);
        for(int k = 0; k < 32; k++)
            if((queue & (1 << k)) != 0)
                traversal4(sv, parentTile * ivec2(4, 8) + ivec2(k % 4, k / 4));
    }
}

// Level 2
void traversal2(ShadowVolume sv, ivec2 parentTile)
{
    const int level = 2;
    ivec2 tile = parentTile * ivec2(8, 4) + ivec2(gl_ThreadInWarpNV % 8, gl_ThreadInWarpNV / 8);
    
    float intersects = testSV(sv, level, tile);
    
    if(intersects > 0.)
        updateShadowBuffer(level, tile);
    else
    {
        uint queue = ballotThreadNV(intersects == 0.);
        for(int k = 0; k < 32; k++)
            if((queue & (1 << k)) != 0)
                traversal3(sv, parentTile * ivec2(8, 4) + ivec2(k % 8, k / 8));
    }
}

// Level 1
void traversal1(ShadowVolume sv, ivec2 parentTile)
{
    const int level = 1;
    ivec2 tile = parentTile * ivec2(4, 8) + ivec2(gl_ThreadInWarpNV % 4, gl_ThreadInWarpNV / 4);
    
    float intersects = testSV(sv, level, tile);
    
    if(intersects > 0.)
        updateShadowBuffer(level, tile);
    else
    {
        uint queue = ballotThreadNV(intersects == 0.);
        for(int k = 0; k < 32; k++)
            if((queue & (1 << k)) != 0)
                traversal2(sv, parentTile * ivec2(4, 8) + ivec2(k % 4, k / 4));
    }
}

// Level 0
void traversal0(ShadowVolume sv)
{
    const int level = 0;
    ivec2 tile = ivec2(gl_ThreadInWarpNV % 8, gl_ThreadInWarpNV / 8);
    
    float intersects = testSV(sv, level, tile);
    if(intersects < 0.)
        updateShadowBuffer(level, tile);
    else
    {
        uint queue = ballotThreadNV(intersects == 0.);
        for(int k = 0; k < 32; k++)
            if((queue & (1 << k)) != 0)
                traversal1(sv, ivec2(k % 8, k / 8));
    }
}

void main()
{
    // There's no way we can ever reach that many tetrahedra but hey you never know right
    // uint invocationIndex = gl_LocalInvocationIndex + workGroupSize *
    //     (gl_WorkGroupID.x + gl_NumWorkGroups.x * (gl_WorkGroupID.y + gl_NumWorkGroups.y * gl_WorkGroupID.z));
    uint cellIndex = gl_GlobalInvocationID.x;
    
    // Build shadow shadow volume
    ShadowVolume sv;
    // Fetch cell-related data
    ivec4 cell = cells[cellIndex];
    mat4 objMV = MV[objIndex[cellIndex]];
    vec4 objMVt = MVt[objIndex[cellIndex]];
    vec4 v[4], center;
    for(int k = 0; k < 4; k++)
        center += (v[k] = objMV * vertices[cell[k]] + objMVt);
    center /= 4.;
    // Build shadow volume's planes as looking away from the centroid of the cell
    for(int k = 0; k < 4; k++)
    {
        sv.planes[k].n = cross4(v[k] - uLightPos, v[(k + 1) % 4] - uLightPos,
            v[(k + 2) % 4] - uLightPos);
        sv.planes[k].n = faceforward(sv.planes[k].n, uLightPos - center, sv.planes[k].n);
        sv.planes[k].c = dot(sv.planes[k].n, uLightPos);
    }
    sv.planes[4].n = normalize(cross4(v[1] - v[0], v[2] - v[0], v[3] - v[0]));
    sv.planes[4].n = faceforward(sv.planes[4].n, uLightPos - v[0], sv.planes[4].n);
    sv.planes[4].c = dot(sv.planes[4].n, v[0]);
    
    traversal0(sv);
}
