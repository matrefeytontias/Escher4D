#version 430

// Intersects 4D geometry with the w = 0 hyperplane and produces triangles
// entirely contained in said hyperplane. The result is 3D geometry with information
// on its 4D provenance.

#define EPSILON 0

layout(lines_adjacency) in; // 4 vertices
layout(triangle_strip, max_vertices = 12) out;

struct Vertex
{
    vec4 pos;
    vec4 normal;
};

uniform mat4 P;

in vec4 vNormal[];
in vec4 vColor[];

out vec4 gPosition;
out vec4 gNormal;
out vec4 gColor;

// Snapped cartesian equation for the hyperplane w = 0
float hyperplane(in Vertex v)
{
    return abs(v.pos.w) < EPSILON ? 0. : v.pos.w;
}

// Returns the amount of linear interpolation from v1 to v2 that gives the
// intersection between the hyperplane w = 0 and the line v1 to v2
float lineSpaceIntersect(in Vertex v1, in Vertex v2)
{
    float u = v2.pos.w - v1.pos.w;
    return abs(u) > EPSILON ? -v1.pos.w / u : -1.;
}

// <u | v> = ||u|| ||v|| cos((u, v))
// Note for later : acos(x) <= acos(y) <=> x >= y
float angleBetween(in vec4 v1, in vec4 v2)
{
    return acos(dot(normalize(v1), normalize(v2)));
}

Vertex interpolate(in Vertex v1, in Vertex v2, float a)
{
    Vertex v;
    v.pos = mix(v1.pos, v2.pos, a);
    v.normal = mix(v1.normal, v2.normal, a);
    return v;
}

Vertex r[4];

void buildVertex(int i)
{
    gPosition = r[i].pos;
    gNormal = r[i].normal;
}

void main()
{
    Vertex v1 = Vertex(gl_in[0].gl_Position, vNormal[0]),
        v2 = Vertex(gl_in[1].gl_Position, vNormal[1]),
        v3 = Vertex(gl_in[2].gl_Position, vNormal[2]),
        v4 = Vertex(gl_in[3].gl_Position, vNormal[3]);
    int i = 0;
    
    float v1v2 = -1., v1v3 = -1., v1v4 = -1., v2v3 = -1., v2v4 = -1., v3v4 = -1.;
    // Intersect tetrahedra
    float s1 = sign(hyperplane(v1)),
            s2 = sign(hyperplane(v2)),
            s3 = sign(hyperplane(v3)),
            s4 = sign(hyperplane(v4));
    
    if(s1 == 0.)
        v1v2 = 0.;
    if(s2 == 0.)
        v2v3 = 0.;
    if(s3 == 0.)
        v3v4 = 0.;
    if(s4 == 0.)
        v1v4 = 1.;
    
    if(s1 * s2 == -1.)
        v1v2 = lineSpaceIntersect(v1, v2);
    if(s1 * s3 == -1.)
        v1v3 = lineSpaceIntersect(v1, v3);
    if(s1 * s4 == -1.)
        v1v4 = lineSpaceIntersect(v1, v4);
    if(s2 * s3 == -1.)
        v2v3 = lineSpaceIntersect(v2, v3);
    if(s2 * s4 == -1.)
        v2v4 = lineSpaceIntersect(v2, v4);
    if(s3 * s4 == -1.)
        v3v4 = lineSpaceIntersect(v3, v4);

    
    if(v1v2 >= 0.)
        r[i++] = interpolate(v1, v2, v1v2);
    if(v1v3 >= 0.)
        r[i++] = interpolate(v1, v3, v1v3);
    if(v1v4 >= 0.)
        r[i++] = interpolate(v1, v4, v1v4);
    if(v2v3 >= 0.)
        r[i++] = interpolate(v2, v3, v2v3);
    if(v2v4 >= 0.)
        r[i++] = interpolate(v2, v4, v2v4);
    if(v3v4 >= 0.)
        r[i++] = interpolate(v3, v4, v3v4);
    
    // Leaving the realm of 4D for the good ol' 3D starting at this point
    if(i == 4)
    {
        vec4 e12 = r[1].pos - r[0].pos, e23 = r[2].pos - r[1].pos;
        if(angleBetween(e12, e23) > angleBetween(e12, r[3].pos - r[1].pos))
        {
            Vertex temp = r[2];
            r[2] = r[3];
            r[3] = temp;
        }
    }
    
    // Projection on the w = 0 hyperplane
    vec3 v1f = r[0].pos.xyz, v2f = r[1].pos.xyz,
        v3f = r[2].pos.xyz, v4f = r[3].pos.xyz;
    vec4 v1p = P * vec4(v1f, 1.),
        v2p = P * vec4(v2f, 1.),
        v3p = P * vec4(v3f, 1.),
        v4p = P * vec4(v4f, 1.);
    
    // "At least" a triangle
    if(i >= 3)
    {
        gl_Position = v1p;
        buildVertex(0);
        EmitVertex();
        gl_Position = v2p;
        buildVertex(1);
        EmitVertex();
        gl_Position = v3p;
        buildVertex(2);
        EmitVertex();
        EndPrimitive();
        
        // Quad or tetrahedron
        if(i == 4)
        {
            gl_Position = v1p;
            buildVertex(0);
            EmitVertex();
            gl_Position = v3p;
            buildVertex(2);
            EmitVertex();
            gl_Position = v4p;
            buildVertex(3);
            EmitVertex();
            EndPrimitive();
            
            vec3 n = cross(v2f - v1f, v3f - v1f);
            // Check for tetrahedron
            if(abs(dot(n, v4f - v3f)) > EPSILON)
            {
                gl_Position = v1p;
                buildVertex(0);
                EmitVertex();
                gl_Position = v2p;
                buildVertex(1);
                EmitVertex();
                gl_Position = v4p;
                buildVertex(3);
                EmitVertex();
                EndPrimitive();
                
                gl_Position = v2p;
                buildVertex(1);
                EmitVertex();
                gl_Position = v3p;
                buildVertex(2);
                EmitVertex();
                gl_Position = v4p;
                buildVertex(3);
                EmitVertex();
                EndPrimitive();
            }
        }
    }
}
