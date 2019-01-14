#version 130

uniform sampler2D texPos;
uniform sampler2D texNormal;
uniform sampler2D texColor;

in vec2 vTexCoord;

out vec4 fragColor;

void main()
{
    vec4 pos = texture(texPos, vTexCoord),
        normal = texture(texNormal, vTexCoord),
        color = texture(texColor, vTexCoord);
    fragColor = color;
}
