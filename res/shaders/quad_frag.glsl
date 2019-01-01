#version 130

in vec3 vPosition;
in vec3 vNormal;

out vec4 fragColor;

void main()
{
    fragColor = vec4(max(0, dot(normalize(vNormal), -normalize(vPosition))));
}
