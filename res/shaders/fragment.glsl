#version 330

layout(location = 0) out vec4 fragPosition;
layout(location = 1) out vec4 fragNormal;
layout(location = 2) out vec4 fragColor;

uniform float uLightIntensity;
uniform float uLightRadius;
uniform vec4 uColor;

in vec4 gPosition;
in vec4 gNormal;

void main()
{
    fragPosition = gPosition;
    fragNormal = gNormal;
    // Real Shading in Unreal Engine 4
    // Inverse square falloff with radius clamping
    float sqd = dot(gPosition, gPosition);
    float falloff = clamp(1 - sqd * sqd / pow(uLightRadius, 4), 0, 1);
    falloff *= falloff / (1 + sqd);
    fragColor = min(1, uLightIntensity * falloff) * uColor;
}
