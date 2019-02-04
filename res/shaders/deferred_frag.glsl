#version 430

uniform sampler2D texPos;
uniform sampler2D texNorm;
uniform sampler2D texColor;
uniform sampler2D texDepth;

uniform float uLightIntensity;
uniform float uLightRadius;
uniform vec4 uLightPos;

uniform bool uDisplayDepth;
uniform int uDepthLevel;
uniform bool uMin;

uniform ivec2 uTexSize;

layout(std430, binding = 5) buffer depthHierarchy
{
    vec2 level0[8 * 4], // one 8*4 tile
        level1[32 * 32], // 8*4 4*8 tiles
        level2[256 * 128], // 32*32 8*4 tiles
        level3[1024 * 1024]; // 256*128 4*8 tiles
};

layout(std430, binding = 6) buffer shadowBuffer
{
    uint slevel0[8 * 4 >> 5],
        slevel1[32 * 32 >> 5],
        slevel2[256 * 128 >> 5],
        slevel3[1024 * 1024 >> 5],
        slevel4[];
};

const ivec2 factors[5] = ivec2[5](ivec2(8, 4), ivec2(32, 32), ivec2(256, 128),
    ivec2(1024, 1024), ivec2(8192, 4096));

in vec2 vTexCoord;

out vec4 fragColor;

bool testShadow(int level, vec2 normCoord)
{
    ivec2 bufferCoord = ivec2(normCoord * factors[level]);
    int offset = bufferCoord.y * (level > 3 ? uTexSize.x : factors[level].x) + bufferCoord.x;
    uint mask;
    
    switch(level)
    {
    case 0:
        mask = slevel0[offset >> 5];
        break;
    case 1:
        mask = slevel1[offset >> 5];
        break;
    case 2:
        mask = slevel2[offset >> 5];
        break;
    case 3:
        mask = slevel3[offset >> 5];
        break;
    case 4:
        mask = slevel4[offset >> 5];
    }
    
    return (mask & (1 << 31 - (offset & 0x1f))) != 0;
}

void main()
{
    vec4 pos = texture(texPos, vTexCoord),
        normal = texture(texNorm, vTexCoord),
        color = texture(texColor, vTexCoord);
    
    vec4 deferredColor;
    
    vec2 normCoord = vTexCoord * uTexSize / vec2(8192., 4096.);
    
    if(!uDisplayDepth)
    {
        // Real Shading in Unreal Engine 4
        // Inverse square falloff with radius clamping
        vec4 lightD = uLightPos - pos;
        float sqd = dot(lightD, lightD);
        float falloff = clamp(1 - sqd * sqd / pow(uLightRadius, 4), 0, 1);
        falloff *= falloff / (1 + sqd);
        deferredColor = min(1, uLightIntensity * falloff) * abs(dot(normal, normalize(lightD))) * color;
    }
    else
    {
        // Fetch depth value
        const float zNear = 1e-4, zFar = 100.;
        vec2 depth;
        ivec2 bufferCoord = ivec2(normCoord * factors[uDepthLevel]);
        switch(uDepthLevel)
        {
        case 0:
            depth = level0[bufferCoord.y * 8 + bufferCoord.x];
            break;
        case 1:
            depth = level1[bufferCoord.y * 32 + bufferCoord.x];
            break;
        case 2:
            depth = level2[bufferCoord.y * 256 + bufferCoord.x];
            break;
        case 3:
            depth = level3[bufferCoord.y * 1024 + bufferCoord.x];
            break;
        case 4:
            depth = texture(texDepth, vTexCoord).rr;
        }
        
        deferredColor = vec4(2. * zNear / (zFar + zNear - (uMin ? depth.x : depth.y) * (zFar - zNear)));
    }
    
    for(int k = 0; k < 5; k++)
    {
        if(testShadow(k, normCoord))
        {
            deferredColor = vec4(0.);
            break;
        }
    }
    
    fragColor = deferredColor;
}
