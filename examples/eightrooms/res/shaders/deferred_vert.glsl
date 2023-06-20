#version 430

in vec2 aPosition;

out vec2 vTexCoord;

void main()
{
    vTexCoord = aPosition / 2. + 0.5;
    gl_Position = vec4(aPosition, 0., 1.);
}
