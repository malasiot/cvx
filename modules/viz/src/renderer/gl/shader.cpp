#include <cvx/viz/renderer/gl/shader.hpp>

#include <cvx/viz/renderer/ogl_shaders.hpp>
#include <cvx/util/misc/format.hpp>
#include <cvx/util/misc/filesystem.hpp>
#include <cvx/util/misc/path.hpp>

#include <cstring>
#include <fstream>

#include "GL/gl3w.h"


using namespace std ;
using namespace Eigen ;
using namespace cvx::util ;

namespace cvx { namespace viz { namespace gl {

void Shader::create(Type t) {
    type_ = t ;

    GLenum shader_type ;
    switch ( type_ ) {
    case Vertex:
        shader_type = GL_VERTEX_SHADER ;
        break ;
    case Fragment:
        shader_type = GL_FRAGMENT_SHADER ;
        break ;
    case Geometry:
        shader_type = GL_GEOMETRY_SHADER ;
        break ;
    case Compute:
        shader_type = GL_COMPUTE_SHADER ;
        break ;
    case TessControl:
        shader_type = GL_TESS_CONTROL_SHADER ;
        break ;
    case TessEvaluation:
        shader_type = GL_TESS_EVALUATION_SHADER ;
    }

    if ( ( handle_ = glCreateShader(GLenum(shader_type)) ) == 0 )
        throw OpenGLShaderError("cannot create shader") ;
}

Shader::Shader(Type t, const std::string &code, const string &rname) {
    create(t) ;
    addSourceString(code, rname) ;
}

void Shader::setHeader(const string &header) {
    header_ = header ;
}

void Shader::addPreProcDefinition(const string &key, const string &val) {
    preproc_ += "#define " + key ;
    if ( !val.empty() ) preproc_ += val ;
    preproc_ += '\n' ;
}

void Shader::addSourceString(const std::string &code, const string &resource_name) {
    sources_.emplace_back(sources_.size(), resource_name, code) ;
}

void Shader::compile() {

    if ( compiled_ ) return ;

    std::unique_ptr<const GLchar* [] > sources(new const GLchar * [sources_.size()]);
    std::unique_ptr< GLint []> lengths(new GLint [sources_.size()]) ;

    for ( uint i=0 ; i<sources_.size() ; i++ ) {
        const Source &src = sources_[i] ;
        sources[i] = src.data_.c_str() ;
        lengths[i] = src.data_.length() ;
    }

    glShaderSource(handle_, sources_.size(), sources.get(), lengths.get());
    glCompileShader(handle_);

    GLint success;
    glGetShaderiv(handle_, GL_COMPILE_STATUS, &success);

    if ( !success ) {
        GLchar info_log[1024];
        glGetShaderInfoLog(handle_, 1024, NULL, info_log);

        throw ShaderError(format("Error compilining shader: {}", info_log)) ;
    }

    compiled_ = true ;
    sources_.clear() ;
}

void Shader::addSourceFile(const std::string &fname, const string &resource_name) {

    string contents = get_file_contents(fname) ;

    if ( contents.empty() ) {
        throw ShaderError(format("Error reading shader file: {}", fname)) ;
    }

    if ( resource_name.empty() )
        addSourceString(contents, Path(fname).name()) ;
    else
        addSourceString(contents, resource_name) ;
}

Shader::~Shader() {
    glDeleteShader(handle_) ;
}

ShaderProgram::ShaderProgram(const char *vshader_code, const char *fshader_code)
{
    handle_ = glCreateProgram();

    Shader::Ptr vertex_shader = make_shared<Shader>(Shader::Vertex, vshader_code) ;
    Shader::Ptr fragment_shader = make_shared<Shader>(Shader::Fragment, fshader_code) ;

    addShader(vertex_shader) ;
    addShader(fragment_shader) ;

    link();
}

void ShaderProgram::setUniform(const string &name, float v)
{
    GLint loc = glGetUniformLocation(handle_, name.c_str()) ;
    if ( loc != -1 ) glUniform1f(loc, v) ;
}

void ShaderProgram::setUniform(const string &name, GLuint v)
{
    GLint loc = glGetUniformLocation(handle_, name.c_str()) ;
    if ( loc != -1 ) glUniform1ui(loc, v) ;
}

void ShaderProgram::setUniform(const string &name, GLint v)
{
    GLint loc = glGetUniformLocation(handle_, name.c_str()) ;
    if ( loc != -1 ) glUniform1i(loc, v) ;
}

void ShaderProgram::setUniform(const string &name, const Vector3f &v)
{
    GLint loc = glGetUniformLocation(handle_, name.c_str()) ;
    if ( loc != -1 ) glUniform3fv(loc, 1, v.data()) ;
}

void ShaderProgram::setUniform(const string &name, const Vector2f &v)
{
    GLint loc = glGetUniformLocation(handle_, name.c_str()) ;
    if ( loc != -1 ) glUniform2fv(loc, 1, v.data()) ;
}

void ShaderProgram::setUniform(const string &name, const Vector4f &v)
{
    GLint loc = glGetUniformLocation(handle_, name.c_str()) ;
    if ( loc != -1 ) glUniform4fv(loc, 1, v.data()) ;
}

void ShaderProgram::setUniform(const string &name, const Matrix3f &v)
{
    GLint loc = glGetUniformLocation(handle_, name.c_str()) ;
    if ( loc != -1 ) glUniformMatrix3fv(loc, 1, GL_FALSE, v.data()) ;
}

void ShaderProgram::setUniform(const string &name, const Matrix4f &v)
{
    GLint loc = glGetUniformLocation(handle_, name.c_str()) ;
    if ( loc != -1 ) glUniformMatrix4fv(loc, 1, GL_FALSE, v.data()) ;
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(handle_) ;
}

ShaderProgram::ShaderProgram() {
    handle_ = glCreateProgram() ;
}

void ShaderProgram::addShader(Shader::Ptr &shader) {
    shader->compile() ;
    shaders_.push_back(shader) ;
}

void ShaderProgram::addShaderFromString(Shader::Type t, const string &code, const string &resource_name) {
    auto shader = std::make_shared<Shader>(t) ;
    shader->addSourceString(code, resource_name) ;
    shader->compile() ;
    addShader(shader) ;
}

void ShaderProgram::addShaderFromFile(Shader::Type t, const string &fname, const string &resource_name) {
    auto shader = std::make_shared<Shader>(t) ;
    shader->addSourceFile(fname, resource_name) ;
    shader->compile() ;
    addShader(shader) ;
}

void ShaderProgram::link(bool validate) {

#if 0
    const GLchar* feedbackVaryings[] = { "gl_Position" };
    glTransformFeedbackVaryings(handle_, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
#endif

    for( auto &&shader: shaders_ )
        glAttachShader(handle_, shader->handle()) ;

    GLchar error_log[1024] = { 0 };

    glLinkProgram(handle_) ;

    GLint success;
    glGetProgramiv(handle_, GL_LINK_STATUS, &success);

    if ( success == 0 ) {
        glGetProgramInfoLog(handle_, sizeof(error_log), NULL, error_log);
        throwError("Error linking shader program", error_log) ;
    }

    if ( validate ) {
        glValidateProgram(handle_);
        glGetProgramiv(handle_, GL_VALIDATE_STATUS, &success);

        if ( !success ) {
            glGetProgramInfoLog(handle_, sizeof(error_log), NULL, error_log);
            throwError("Invalid shader program", error_log);
        }
    }
}

void ShaderProgram::use() {
   glUseProgram(handle_) ;
}

void ShaderProgram::throwError(const char *error_str, const char *error_desc) {
    throw OpenGLShaderError(format("{}: {}", error_str, error_desc)) ;
}

OpenGLShader::Type type_from_string(const string &s) {
    if ( s == "vertex" ) return OpenGLShader::Vertex ;
    else if ( s == "fragment" ) return OpenGLShader::Fragment ;
    else if ( s == "geometry" ) return OpenGLShader::Geometry ;
    else if ( s == "compute" ) return OpenGLShader::Compute ;
    else if ( s == "tess_control" ) return OpenGLShader::TessControl ;
    else if ( s == "tess_evaluation" ) return OpenGLShader::TessEvaluation ;
    else return OpenGLShader::Fragment ;
}


ShaderError::ShaderError(const string &msg):
    std::runtime_error(msg) {}

} // gl
} // viz
} // cvx
