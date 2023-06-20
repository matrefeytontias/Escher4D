#ifndef INC_UTILS
#define INC_UTILS

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

#include <glad/glad.h>

#ifdef _WIN32
#undef near
#undef far
#endif

#define M_PI 3.1415926535897932384626433832795f

namespace Empty::math
{
    template <typename T> struct _mat4;
    using mat4 = _mat4<float>;
}

/**
 * Returns the contents of a file as a string. Raises a runtime_error exception
 * upon failure.
 */
std::string getFileContents(const std::string &path);
/**
 * Splits a string on delimiting characters. The delimiting characters are a disjunction,
 * meaning either of the characters in the delimiter string is to split the string.
 */
std::vector<std::string> split(const std::string &s, const std::string &delim);
/**
 * Creates an OpenGL shader from the path of its source file.
 * @param   type    the OpenGL type of the shader (eg vertex, fragment ...)
 * @param   path    the path of the source file
 * @return  a valid OpenGL shader name
 */
GLuint createShaderFromSource(GLenum type, const std::string &path);

/**
 * Print the shader log associated with a shader name.
 */
void printShaderLog(GLuint shader);

/**
 * Prints data to std::cerr. Use the operator<< notation to print sequentially.
 */
#define trace(s) std::cerr << __FILE__ << ":" << __LINE__ << " : " << s << std::endl

/**
 * Raises a runtime_error exception with the given message. Use the operator<< notation to print sequentially.
 */
#define fatal(s) do { std::stringstream ss; ss << __FILE__ << ":" << __LINE__ << " : " << s << std::endl; throw std::runtime_error(ss.str()); } while(0)

/**
 * Constructs a 4x4 3D perspective matrix in the given matrix object based on FOV,
 * aspect ratio, near plane and far plane.
 */
void perspective(Empty::math::mat4 &p, float fov, float ratio, float near, float far);
/**
 * Shorthand function to modify the aspect ratio of a perspective matrix.
 */
void setAspectRatio(Empty::math::mat4 &p, float ratio);

/**
 * Shorthand function to display an OpenGL texture as a quarter of the screen.
 */
void displayTexture(GLint texture, float dx = 0.f, float dy = 0.f);

#endif
