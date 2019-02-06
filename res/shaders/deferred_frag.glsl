#version 430

uniform sampler2D texPos;
uniform sampler2D texNorm;
uniform sampler2D texColor;

uniform float uLightIntensity;
uniform float uLightRadius;
uniform vec4 uLightPos;

uniform ivec2 uTexSize;

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
    
    // Real Shading in Unreal Engine 4
    // Inverse square falloff with radius clamping
    vec4 lightD = uLightPos - pos;
    float sqd = dot(lightD, lightD);
    float falloff = clamp(1 - sqd * sqd / pow(uLightRadius, 4), 0, 1);
    falloff *= falloff / (1 + sqd);
    
    vec4 deferredColor = min(1, uLightIntensity * falloff) * abs(dot(normal, normalize(lightD))) * color;
    
    vec2 normCoord = vTexCoord * uTexSize / vec2(8192., 4096.);
    
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
