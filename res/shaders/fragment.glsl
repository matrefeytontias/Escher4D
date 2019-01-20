#version 430

layout(location = 0) out vec4 fragPosition;
layout(location = 1) out vec4 fragNormal;
layout(location = 2) out vec3 fragColor;

uniform vec4 uColor;

in vec4 gPosition;
in vec4 gNormal;

void main()
{
    fragPosition = gPosition;
    fragNormal = normalize(gNormal);
    fragColor = uColor.rgb;
}
