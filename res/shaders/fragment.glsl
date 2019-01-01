#version 150

uniform float uLightIntensity;
uniform float uLightRadius;
uniform vec4 uColor;

in vec4 gPosition;
in vec4 gNormal;

out vec4 fragColor;

void main()
{
    // Real Shading in Unreal Engine 4
    // Inverse square falloff with radius clamping
    float sqd = dot(gPosition, gPosition);
    float falloff = clamp(1 - sqd * sqd / pow(uLightRadius, 4), 0, 1);
    falloff *= falloff / (1 + sqd);
    fragColor = min(1, uLightIntensity * falloff) * max(0, -dot(normalize(gNormal), normalize(gPosition))) * uColor;
}
