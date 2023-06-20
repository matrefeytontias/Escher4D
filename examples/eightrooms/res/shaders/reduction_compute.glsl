#version 430

uniform sampler2D texPos;
uniform ivec2 uTexSize;

// Each tile holds the min and max depth of the subtiles in the level under it.
// Each tile holds 32 items and is 8*4 or 4*8 depending on the level. Conceptually,
// the texPos texture is level 4.
layout(std430, binding = 5) buffer aabbHierarchy
{
    vec4 minlevel0[8 * 4], // one 8*4 tile
        minlevel1[32 * 32], // 8*4 4*8 tiles
        minlevel2[256 * 128], // 32*32 8*4 tiles
        minlevel3[1024 * 1024]; // 256*128 4*8 tiles
    vec4 maxlevel0[8 * 4],
        maxlevel1[32 * 32],
        maxlevel2[256 * 128],
        maxlevel3[1024 * 1024];
};

const float POS_INF = 1e5;
const vec4 dummy = vec4(POS_INF);

layout(local_size_x = 32) in;

// Every even index holds the min, every odd index holds the max
shared vec4 aabbFetch[64];

// Parallely reduce 32-item tile into the first cell
void reduceGroup(uint tid)
{
    for(int stride = 32; stride > 1; stride >>= 1)
    {
        if(tid < stride)
        {
            const vec4 dm = aabbFetch[tid], odm = aabbFetch[tid + stride],
                dM = aabbFetch[tid + 1], odM = aabbFetch[tid + stride + 1];
            aabbFetch[tid] = min(dm, odm);
            aabbFetch[tid + 1] = max(dM, odM);
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
    {
        aabbFetch[tid * 2] = dummy;
        aabbFetch[tid * 2 + 1] = -dummy;
    }
    else
    {
        vec4 f = texelFetch(texPos, texel, 0);
        aabbFetch[tid * 2] = f;
        aabbFetch[tid * 2 + 1] = f;
    }
    memoryBarrierShared();
    
    reduceGroup(tid * 2);
    if(tid == 0)
    {
        int offset = (parentTile.y << 10) + parentTile.x;
        minlevel3[offset] = aabbFetch[0];
        maxlevel3[offset] = aabbFetch[1];
    }
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
    if(texel.y >= texSizeNextLevel.y || texel.x >= texSizeNextLevel.x)
    {
        aabbFetch[tid * 2] = dummy;
        aabbFetch[tid * 2 + 1] = -dummy;
    }
    else
    {
        int offset = (texel.y << 10) + texel.x;
        aabbFetch[tid * 2] = minlevel3[offset];
        aabbFetch[tid * 2 + 1] = maxlevel3[offset];
    }
    memoryBarrierShared();
    
    reduceGroup(tid * 2);
    if(tid == 0)
    {
        int offset = (parentTile.y << 8) + parentTile.x;
        minlevel2[offset] = aabbFetch[0];
        maxlevel2[offset] = aabbFetch[1];
    }
    memoryBarrierBuffer();
    
    // Build level 1 ; level 2 tiles are 8x4
    texSizeNextLevel = texSizeLevel;
    texSizeLevel = (texSizeLevel + ivec2(7, 3)) / ivec2(8, 4);
    if(parentTile.x >= texSizeLevel.x || parentTile.y >= texSizeLevel.y)
        return;
    texel = parentTile * ivec2(8, 4) + ivec2(tid & 7, tid >> 3);
    if(texel.y >= texSizeNextLevel.y || texel.x >= texSizeNextLevel.x)
    {
        aabbFetch[tid * 2] = dummy;
        aabbFetch[tid * 2 + 1] = -dummy;
    }
    else
    {
        int offset = (texel.y << 8) + texel.x;
        aabbFetch[tid * 2] = minlevel2[offset];
        aabbFetch[tid * 2 + 1] = maxlevel2[offset];
    }
    memoryBarrierShared();
    
    reduceGroup(tid * 2);
    if(tid == 0)
    {
        int offset = (parentTile.y << 5) + parentTile.x;
        minlevel1[offset] = aabbFetch[0];
        maxlevel1[offset] = aabbFetch[1];
    }
    memoryBarrierBuffer();
    
    // Build level 0 ; level 1 tiles are 4x8
    texSizeNextLevel = texSizeLevel;
    texSizeLevel = (texSizeLevel + ivec2(3, 7)) / ivec2(4, 8);
    if(parentTile.x >= texSizeLevel.x || parentTile.y >= texSizeLevel.y)
        return;
    texel = parentTile * ivec2(4, 8) + ivec2(tid & 3, tid >> 2);
    if(texel.y >= texSizeNextLevel.y || texel.x >= texSizeNextLevel.x)
    {
        aabbFetch[tid * 2] = dummy;
        aabbFetch[tid * 2 + 1] = -dummy;
    }
    else
    {
        int offset = (texel.y << 5) + texel.x;
        aabbFetch[tid * 2] = minlevel1[offset];
        aabbFetch[tid * 2 + 1] = maxlevel1[offset];
    }
    memoryBarrierShared();
    
    reduceGroup(tid * 2);
    if(tid == 0)
    {
        int offset = (parentTile.y << 3) + parentTile.x;
        minlevel0[offset] = aabbFetch[0];
        maxlevel0[offset] = aabbFetch[1];
    }
    
    // Done !
}
