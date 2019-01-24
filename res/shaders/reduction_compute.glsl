#version 430

uniform sampler2D texDepth;
uniform ivec2 texSize;

// Each tile holds the min and max depth of the subtiles in the level under it.
// Each tile holds 32 items and is 8*4 or 4*8 depending on the level. Conceptually,
// texDepth is level 4.
layout(std430, binding = 5) buffer depthHierarchy
{
    vec2 level0[8 * 4], // one 8*4 tile
        level1[32 * 32], // 8*4 4*8 tiles
        level2[256 * 128], // 32*32 8*4 tiles
        level3[1024 * 1024]; // 256*128 4*8 tiles
};

layout(local_size_x = 32) in;

const ivec2 tileSizes[5] = ivec2[5](ivec2(8, 4), ivec2(4, 8), ivec2(8, 4), ivec2(4, 8), ivec2(8, 4)),
    tilePowers[5] = ivec2[5](ivec2(3, 2), ivec2(2, 3), ivec2(3, 2), ivec2(2, 3), ivec2(3, 2));

shared vec2 depthFetch[32];

/**
 * Parallel reduction of texDepth into depthHierarchy.
 * The algorithm fills the levels of the depth hierarchy in a finitely recursive
 * manner to calculate the min/max depth of each tile's subtiles.
 */
void main()
{
    uint tid = gl_LocalInvocationID.x;
    
    // Gather the level 4 subtiles' value
    const ivec2 tileSize = tileSizes[4], tilePower = tilePowers[4];
    const ivec2 texel = ivec2(gl_WorkGroupID.xy) * tileSize + ivec2(tid & (tileSize.x - 1), tid >> tilePower.x);
    
    depthFetch[tid] = texel.x >= texSize.x || texel.y >= texSize.y
        ? vec2(1., 0.)
        : texelFetch(texDepth, texel, 0).rr;
    
    memoryBarrierShared();
    
    // Reduce 32-item tile into level 3
    for(int stride = 16; stride > 0; stride >>= 1)
    {
        if(tid < stride)
        {
            const vec2 d = depthFetch[tid], od = depthFetch[tid + stride];
            depthFetch[tid] = vec2(min(d.x, od.x), max(d.y, od.y));
        }
        memoryBarrierShared();
    }
    
    level3[(gl_WorkGroupID.y << 10) + gl_WorkGroupID.x] = depthFetch[0];
}
