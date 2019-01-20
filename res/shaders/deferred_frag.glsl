#version 430

uniform sampler2D texPos;
uniform sampler2D texNorm;
uniform sampler2D texColor;

uniform float uLightIntensity;
uniform float uLightRadius;
uniform vec4 uLightPos;

in vec2 vTexCoord;

out vec4 fragColor;

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
    fragColor = min(1, uLightIntensity * falloff) * abs(dot(normal, normalize(lightD))) * color;
}
