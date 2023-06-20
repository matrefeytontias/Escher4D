#version 430

uniform mat4 MV;
uniform mat4 tinvMV;
uniform vec4 MVt;
uniform bool uInsideOut;

in vec4 aPosition;
in vec4 aNormal;

out vec4 vNormal;

void main()
{
    gl_Position = MV * aPosition + MVt;
    vNormal = normalize(tinvMV * (uInsideOut ? -aNormal : aNormal));
}
