#include <iostream>
#include <map>

#include "ShaderProgram.hpp"
#include "utils.hpp"

std::map<GLuint, int> Texture::_pool;

ShaderProgram::ShaderProgram()
{
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    
    _program = glCreateProgram();
    _dirty = true;
}

ShaderProgram::~ShaderProgram()
{
    glBindVertexArray(_vao);
    
    for(auto cpl : _attributes)
        glDisableVertexAttribArray(cpl.second);
    
    for(GLenum i : _shaders)
    {
        glDetachShader(_program, i);
        glDeleteShader(i);
    }
    glDeleteProgram(_program);
    glDeleteVertexArrays(1, &_vao);
}

void ShaderProgram::attachFromSource(GLenum type, const char *src)
{
    GLenum i = glCreateShader(type);
    glShaderSource(i, 1, &src, NULL);
    glCompileShader(i);
    printShaderLog(i);
    glAttachShader(_program, i);
    _shaders.push_back(i);
    _dirty = true;
}

void ShaderProgram::attach(GLenum type, const char *path)
{
    GLenum i = createShaderFromSource(type, path);
    printShaderLog(i);
    glAttachShader(_program, i);
    _shaders.push_back(i);
    _dirty = true;
}

void ShaderProgram::use()
{
    if(_dirty)
    {
        glLinkProgram(_program);
        _dirty = false;
    }
    glBindVertexArray(_vao);
    glUseProgram(_program);
    int i = 0;
    for(auto tex : _textures)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(tex.second.target, tex.second.id);
        glUniform1i(ensureUniform(tex.first), i++);
    }
}

void ShaderProgram::uniform1f(const string &name, float v)
{
    glUniform1f(ensureUniform(name), v);
}

void ShaderProgram::uniform2f(const string &name, float v1, float v2)
{
    glUniform2f(ensureUniform(name), v1, v2);
}

void ShaderProgram::uniform3f(const string &name, float v1, float v2, float v3)
{
    glUniform3f(ensureUniform(name), v1, v2, v3);
}

void ShaderProgram::uniform4f(const string &name, float v1, float v2, float v3, float v4)
{
    glUniform4f(ensureUniform(name), v1, v2, v3, v4);
}

void ShaderProgram::uniformMatrix4fv(const string &name, GLuint count, const GLfloat *v)
{
    glUniformMatrix4fv(ensureUniform(name), count, GL_FALSE, v);
}

void ShaderProgram::uniformMatrix3fv(const string &name, GLuint count, const GLfloat *v)
{
    glUniformMatrix3fv(ensureUniform(name), count, GL_FALSE, v);
}

void ShaderProgram::vertexAttribPointer(const string &name, GLuint size, GLenum type, GLsizei stride, const GLuint offset)
{
    glEnableVertexAttribArray(ensureAttrib(name));
    glVertexAttribPointer(_attributes[name], size, type, GL_FALSE, stride, reinterpret_cast<GLvoid*>(offset));
}

Texture &ShaderProgram::getTexture(const string &name)
{
    if(_textures.find(name) == _textures.end())
    {
        Texture tex;
        _textures[name] = tex;
    }
    return _textures[name];
}

Texture &ShaderProgram::registerTexture(const string &name, const Texture &tex)
{
    return _textures[name] = tex;
}

GLint ShaderProgram::ensureUniform(const string &name)
{
    if(_uniforms.find(name) == _uniforms.end())
        _uniforms[name] = glGetUniformLocation(_program, name.c_str());
    return _uniforms[name];
}

GLint ShaderProgram::ensureAttrib(const string &name)
{
    if(_attributes.find(name) == _attributes.end())
        _attributes[name] = glGetAttribLocation(_program, name.c_str());
    return _attributes[name];
}