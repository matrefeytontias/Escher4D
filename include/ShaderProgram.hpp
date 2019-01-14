#ifndef INC_SHADER_PROGRAM
#define INC_SHADER_PROGRAM

#include <map>
#include <string>
#include <vector>

#include <glad/glad.h>

#include "utils.hpp"

using namespace std;

struct Texture
{
    GLuint id = 0;
    GLenum target = GL_TEXTURE_2D;
    
    Texture()
    {
        glGenTextures(1, &id);
        registerTex(*this);
    }
    
    Texture(const Texture &t)
    {
        id = t.id;
        target = t.target;
        registerTex(*this);
    }
    
    Texture &operator=(const Texture &t)
    {
        unregisterTex(*this);
        id = t.id;
        target = t.target;
        registerTex(*this);
        return *this;
    }
    
    ~Texture()
    {
        unregisterTex(*this);
    }
private:
    static void registerTex(const Texture &t)
    {
        if(_pool.find(t.id) == _pool.end())
            _pool[t.id] = 1;
        else
            ++_pool[t.id];
    }
    static void unregisterTex(const Texture &t)
    {
        if(!--_pool[t.id])
        {
            glDeleteTextures(1, &t.id);
            _pool.erase(t.id);
        }
    }
    static map<GLuint, int> _pool;
};

class ShaderProgram
{
public:
    ShaderProgram();
    ~ShaderProgram();
    void attach(GLenum type, const char *path);
    void attachFromSource(GLenum type, const char *src);
    void use();
    void uniform1f(const string &name, float v);
    void uniform2f(const string &name, float v1, float v2);
    void uniform3f(const string &name, float v1, float v2, float v3);
    void uniform4f(const string &name, float v1, float v2, float v3, float v4);
    void uniformMatrix4fv(const string &name, GLuint count, const GLfloat *v);
    void uniformMatrix3fv(const string &name, GLuint count, const GLfloat *v);
    void vertexAttribPointer(const string &name, GLuint size, GLenum type, GLsizei stride, const GLuint offset);
    Texture &getTexture(const string &name);
    Texture &registerTexture(const string &name, const Texture &tex);
    unsigned int getTexturesAmount() const { return _textures.size(); }
    GLint ensureUniform(const string &name);
    GLint ensureAttrib(const string &name);
private:
    bool _dirty;
    GLuint _vao, _program;
    vector<GLenum> _shaders;
    map<string, GLint> _uniforms;
    map<string, GLint> _attributes;
    map<string, Texture> _textures;
    static GLuint _currentProgram;
};

#endif
