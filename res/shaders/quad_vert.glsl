#version 130

uniform float uTime;
uniform mat4 uP;

in vec2 aPosition;
in vec3 aNormal;

out vec3 vPosition;
out vec3 vNormal;

void main()
{
    vec3 pos = vec3(aPosition, 0.);
    mat3 rot = mat3(cos(uTime), 0., -sin(uTime),
        0., 1., 0.,
        sin(uTime), 0., cos(uTime));
    rot *= mat3(1., 0., 0.,
        0., cos(uTime), -sin(uTime),
        0., sin(uTime), cos(uTime));
    vPosition = rot * pos + vec3(0., 0., -5.);
    vNormal = rot * aNormal;
    gl_Position = uP * vec4(vPosition, 1.);
}
