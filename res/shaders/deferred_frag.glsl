#version 130

uniform sampler2D texPos;
uniform sampler2D texNorm;
uniform sampler2D texColor;

in vec2 vTexCoord;

out vec4 fragColor;

void main()
{
    vec4 pos = texture(texPos, vTexCoord),
        normal = texture(texNorm, vTexCoord),
        color = texture(texColor, vTexCoord);
    fragColor = abs(dot(normal, normalize(pos))) * color;
}
