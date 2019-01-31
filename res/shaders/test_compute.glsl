#version 430
#extension GL_NV_gpu_shader5 : require
#extension GL_NV_shader_thread_group : require

layout(local_size_x = 32) in;

// Light position in camera space
uniform vec4 uLightPos;

// Inverse projection matrix
uniform mat4 invP;

uniform ivec2 uTexSize;

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
layout(std430, binding = 5) buffer depthHierarchy
{
    vec2 dlevel0[8 * 4], // one 8*4 tile
        dlevel1[32 * 32], // 8*4 4*8 tiles
        dlevel2[256 * 128], // 32*32 8*4 tiles
        dlevel3[1024 * 1024]; // 256*128 4*8 tiles
};
// The shadow buffer has 1 bit per pixel, so use uint and use 32 times fewer bytes.
layout(std430, binding = 6) buffer shadowBuffer
{
    uint slevel0[8 * 4 >> 5],
        slevel1[32 * 32 >> 5],
        slevel2[256 * 128 >> 5],
        slevel3[1024 * 1024 >> 5],
        slevel4[];
};

layout(rgba16f, binding = 0) restrict readonly uniform image2D texPos;

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

bool pointOverHyperplane(in Plane p, in vec4 v)
{
    return dot(v, p.n) - p.c > 0;
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
const ivec2 bufferTileSizes[5] = ivec2[5](ivec2(8, 4), ivec2(32, 32), ivec2(256, 128),
    ivec2(1024, 1024), ivec2(8192, 4096)),
    fragmentTileSizes[5] = ivec2[5](ivec2(1024, 1024), ivec2(256, 128), ivec2(32, 32),
    ivec2(8, 4), ivec2(1, 1));

bool tileInScreen(int level, ivec2 tile)
{
    ivec2 extent = tile * fragmentTileSizes[level];
    return extent.x < uTexSize.x && extent.y < uTexSize.y;
}

vec2 getClipSpaceTileSize(int level)
{
    return fragmentTileSizes[level] * 2. / uTexSize;
}

vec2 getDepthsFromBuffer(int level, ivec2 tile)
{
    switch(level)
    {
    case 0:
        return dlevel0[(tile.y << 3) + tile.x];
    case 1:
        return dlevel1[(tile.y << 5) + tile.x];
    case 2:
        return dlevel2[(tile.y << 8) + tile.x];
    default:
        return dlevel3[(tile.y << 10) + tile.x];
    }
}

// Tests a tile against a shadow volume in view space.
// Returns -1, 0 or +1 depending on the tile being inside, saddled on or outside
// the SV respectively.
float testSV(ShadowVolume sv, int level, ivec2 tile)
{
    vec2 tileSize = getClipSpaceTileSize(level);
    vec4 tileMin, tileMax;
    tileMin.xy = vec2(-1.) + tile * tileSize;
    tileMax.xy = tileMin.xy + tileSize;
    vec2 depths = getDepthsFromBuffer(level, tile);
    tileMin.z = depths.x;
    tileMax.z = depths.y;
    // Go from clip space to view space using homogeneous coordinates
    tileMin.w = tileMax.w = 1;
    tileMin = invP * tileMin;
    tileMax = invP * tileMax;
    tileMin /= tileMin.w;
    tileMax /= tileMax.w;
    // 4D coordinates
    tileMin.w = tileMax.w = 0;
    
    vec4 temp = min(tileMin, tileMax);
    tileMax = max(tileMin, tileMax);
    tileMin = temp;
    
    float result = 0.;
    bool outside = false;
    for(int k = 0; k < 5; ++k)
    {
        float tresult = testHyperplaneAABB(sv.planes[k], tileMin, tileMax);
        outside = outside || tresult > 0;
        result += tresult;
        
    }
    return outside ? 1. : (result == -5 ? -1. : 0.);
}

// Tests a view sample against a shadow volume in view space.
bool testSVsample(ShadowVolume sv, ivec2 tile)
{
    vec4 vs = imageLoad(texPos, tile);
    bool result = false;
    for(int k = 0; k < 5; ++k)
        result = result || pointOverHyperplane(sv.planes[k], vs);
    return !result;
}

// Sets the shadow buffer bit for the given tile at the given level.
void updateShadowBuffer(int level, ivec2 tile)
{
    int offset;
    switch(level)
    {
    case 0:
        offset = (tile.y << 3) + tile.x;
        atomicOr(slevel0[offset >> 5], 1 << (31 - (offset & 0x1f)));
        break;
    case 1:
        offset = (tile.y << 5) + tile.x;
        atomicOr(slevel1[offset >> 5], 1 << (31 - (offset & 0x1f)));
        break;
    case 2:
        offset = (tile.y << 8) + tile.x;
        atomicOr(slevel2[offset >> 5], 1 << (31 - (offset & 0x1f)));
        break;
    case 3:
        offset = (tile.y << 10) + tile.x;
        atomicOr(slevel3[offset >> 5], 1 << (31 - (offset & 0x1f)));
        break;
    default:
        offset = tile.y * uTexSize.x + tile.x;
        atomicOr(slevel4[offset >> 5], 1 << (31 - (offset & 0x1f)));
        break;
    }
}

// Processes a subtile of the given parentTile. Which subtile
// is determined by the lane ID.

shared bool subTileIntersects[32];

// Level 4 (final level)
void traversal4(ShadowVolume sv, ivec2 parentTile)
{
    const int level = 4;
    ivec2 tile = parentTile * ivec2(8, 4) + ivec2(gl_ThreadInWarpNV & 7, gl_ThreadInWarpNV >> 3);
    
    if(!tileInScreen(level, tile))
        return;
    
    if(testSVsample(sv, tile))
        updateShadowBuffer(level, tile);
}

// Level 3
void traversal3(ShadowVolume sv, ivec2 parentTile)
{
    const int level = 3;
    ivec2 tile = parentTile * ivec2(4, 8) + ivec2(gl_ThreadInWarpNV & 3, gl_ThreadInWarpNV >> 2);
    
    float intersects = tileInScreen(level, tile) ? testSV(sv, level, tile) : 1.;
    
    if(intersects < 0.)
        updateShadowBuffer(level, tile);
    
    uint queue = ballotThreadNV(intersects == 0.);
    for(int k = 0; k < 32; k++)
        if((queue & (1 << k)) != 0)
            traversal4(sv, parentTile * ivec2(4, 8) + ivec2(k & 3, k >> 2));
}

// Level 2
void traversal2(ShadowVolume sv, ivec2 parentTile)
{
    const int level = 2;
    ivec2 tile = parentTile * ivec2(8, 4) + ivec2(gl_ThreadInWarpNV & 7, gl_ThreadInWarpNV >> 3);
    
    float intersects = tileInScreen(level, tile) ? testSV(sv, level, tile) : 1.;
    
    if(intersects < 0.)
        updateShadowBuffer(level, tile);
    
    uint queue = ballotThreadNV(intersects == 0.);
    for(int k = 0; k < 32; k++)
        if((queue & (1 << k)) != 0)
            traversal3(sv, parentTile * ivec2(8, 4) + ivec2(k & 7, k >> 3));
}

// Level 1
void traversal1(ShadowVolume sv, ivec2 parentTile)
{
    const int level = 1;
    ivec2 tile = parentTile * ivec2(4, 8) + ivec2(gl_ThreadInWarpNV & 3, gl_ThreadInWarpNV >> 2);
    
    float intersects = tileInScreen(level, tile) ? testSV(sv, level, tile) : 1.;
    
    if(intersects < 0.)
        updateShadowBuffer(level, tile);
    
    uint queue = ballotThreadNV(intersects == 0.);
    
    for(int k = 0; k < 32; k++)
        if((queue & (1 << k)) != 0)
            traversal2(sv, parentTile * ivec2(4, 8) + ivec2(k & 3, k >> 2));
}

// Level 0
void traversal0(ShadowVolume sv)
{
    const int level = 0;
    ivec2 tile = ivec2(gl_ThreadInWarpNV & 7, gl_ThreadInWarpNV >> 3);
    
    float intersects = tileInScreen(level, tile) ? testSV(sv, level, tile) : 1.;
    
    if(intersects < 0.)
        updateShadowBuffer(level, tile);
    
    uint queue = ballotThreadNV(intersects == 0.);
    
    for(int k = 0; k < 32; k++)
        if((queue & (1 << k)) != 0)
            traversal1(sv, ivec2(k & 7, k >> 3));
}

void main()
{
    uint cellIndex = gl_WorkGroupID.x;
    
    // Build shadow volume
    ShadowVolume sv;
    // Fetch cell-related data
    ivec4 cell = cells[cellIndex];
    mat4 objMV = MV[objIndex[cellIndex]];
    vec4 objMVt = MVt[objIndex[cellIndex]];
    vec4 v[4], center = vec4(0.);
    for(int k = 0; k < 4; k++)
    {
        v[k] = objMV * vertices[cell[k]] + objMVt;
        center += v[k];
    }
    center /= 4.;
    // Build shadow volume's planes as looking away from the centroid of the cell
    for(int k = 0; k < 4; ++k)
    {
        sv.planes[k].n = cross4(v[k] - uLightPos, v[(k + 1) & 3] - uLightPos,
            v[(k + 2) & 3] - uLightPos);
        sv.planes[k].n = normalize(sv.planes[k].n * sign(dot(uLightPos - center, sv.planes[k].n)));
        sv.planes[k].c = dot(sv.planes[k].n, uLightPos);
    }
    sv.planes[4].n = cross4(v[1] - v[0], v[2] - v[0], v[3] - v[0]);
    sv.planes[4].n = normalize(sv.planes[4].n * sign(dot(uLightPos - v[0], sv.planes[4].n)));
    // Slightly lower the plane to avoid self-shadowing
    sv.planes[4].c = dot(sv.planes[4].n, v[0] - sv.planes[4].n * 0.001);
    
    barrier();
    traversal0(sv);
}
