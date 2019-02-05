#version 430

uniform sampler2D texPos;
uniform ivec2 uTexSize;

struct AABB
{
    vec4 low, high;
};

// Each tile holds the min and max depth of the subtiles in the level under it.
// Each tile holds 32 items and is 8*4 or 4*8 depending on the level. Conceptually,
// the texPos texture is level 4.
layout(std430, binding = 5) buffer aabbHierarchy
{
    AABB level0[8 * 4], // one 8*4 tile
        level1[32 * 32], // 8*4 4*8 tiles
        level2[256 * 128], // 32*32 8*4 tiles
        level3[1024 * 1024]; // 256*128 4*8 tiles
};

const float POS_INF = 1. / 0.;
const AABB dummyAABB = AABB(vec4(POS_INF), vec4(-POS_INF));

layout(local_size_x = 32) in;

shared AABB aabbFetch[32];

// Parallely reduce 32-item tile into the first cell
void reduceGroup(uint tid)
{
    for(int stride = 16; stride > 0; stride >>= 1)
    {
        if(tid < stride)
        {
            const AABB d = aabbFetch[tid], od = aabbFetch[tid + stride];
            aabbFetch[tid] = AABB(min(d.low, od.low), max(d.high, od.high));
        }
        memoryBarrierShared();
    }
}

// Parallel reduction of texPos into aabbHierarchy.
// The algorithm fills the levels of the depth hierarchy in a finitely recursive
// manner to calculate the min/max depth of each tile's subtiles.
// Each work group's 32 threads process one of the 32 subtiles of each tile.
void main()
{
    const uint tid = gl_LocalInvocationID.x;
    const ivec2 parentTile = ivec2(gl_WorkGroupID.xy);
    
    ivec2 texSizeNextLevel = uTexSize;
    // For this level, clipping against parentTile is done at the dispatch level
    // with dispatchCompute(display_w / 8, display_h / 4, 1).
    // Still, define it here for consistency's sake
    ivec2 texSizeLevel = (texSizeNextLevel + ivec2(7, 3)) / ivec2(8, 4);
    
    // Gather the level 4 tiles' value
    // Tiles are 8x4 in level 4
    ivec2 texel = parentTile * ivec2(8, 4) + ivec2(tid & 7, tid >> 3);
    
    if(texel.y >= texSizeNextLevel.y || texel.x >= texSizeNextLevel.x)
        aabbFetch[tid] = dummyAABB;
    else
    {
        vec4 f = texelFetch(texPos, texel, 0);
        aabbFetch[tid] = AABB(f, f);
    }
    memoryBarrierShared();
    
    reduceGroup(tid);
    if(tid == 0)
        level3[(parentTile.y << 10) + parentTile.x] = aabbFetch[0];
    memoryBarrierBuffer();
    
    // Reduce into level 2, then 1, then 0 in the same way every time
    // discarding out-of-bounds threads in the process. Tiles are respectively
    // 4x8, 8x4 and 4x8 in levels 3, 2 and 1
    
    // Build level 2 ; level 3 tiles are 4x8
    texSizeNextLevel = texSizeLevel;
    texSizeLevel = (texSizeLevel + ivec2(3, 7)) / ivec2(4, 8);
    if(parentTile.x >= texSizeLevel.x || parentTile.y >= texSizeLevel.y)
        return;
    texel = parentTile * ivec2(4, 8) + ivec2(tid & 3, tid >> 2);
    aabbFetch[tid] = texel.y >= texSizeNextLevel.y || texel.x >= texSizeNextLevel.x
        ? dummyAABB
        : level3[(texel.y << 10) + texel.x];
    memoryBarrierShared();
    
    reduceGroup(tid);
    if(tid == 0)
        level2[(parentTile.y << 8) + parentTile.x] = aabbFetch[0];
    memoryBarrierBuffer();
    
    // Build level 1 ; level 2 tiles are 8x4
    texSizeNextLevel = texSizeLevel;
    texSizeLevel = (texSizeLevel + ivec2(7, 3)) / ivec2(8, 4);
    if(parentTile.x >= texSizeLevel.x || parentTile.y >= texSizeLevel.y)
        return;
    texel = parentTile * ivec2(8, 4) + ivec2(tid & 7, tid >> 3);
    aabbFetch[tid] = texel.y >= texSizeNextLevel.y || texel.x >= texSizeNextLevel.x
        ? dummyAABB
        : level2[(texel.y << 8) + texel.x];
    memoryBarrierShared();
    
    reduceGroup(tid);
    if(tid == 0)
        level1[(parentTile.y << 5) + parentTile.x] = aabbFetch[0];
    memoryBarrierBuffer();
    
    // Build level 0 ; level 1 tiles are 4x8
    texSizeNextLevel = texSizeLevel;
    texSizeLevel = (texSizeLevel + ivec2(3, 7)) / ivec2(4, 8);
    if(parentTile.x >= texSizeLevel.x || parentTile.y >= texSizeLevel.y)
        return;
    texel = parentTile * ivec2(4, 8) + ivec2(tid & 3, tid >> 2);
    aabbFetch[tid] = texel.y >= texSizeNextLevel.y || texel.x >= texSizeNextLevel.x
        ? dummyAABB
        : level1[(texel.y << 5) + texel.x];
    memoryBarrierShared();
    
    reduceGroup(tid);
    if(tid == 0)
        level0[(parentTile.y << 3) + parentTile.x] = aabbFetch[0];
    
    // Done !
}
