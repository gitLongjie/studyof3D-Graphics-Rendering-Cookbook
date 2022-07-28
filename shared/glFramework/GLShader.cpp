#include "GLShader.h"
#include "shared/Utils.h"

#include <glad/gl.h>
#include <assert.h>
#include <stdio.h>
#include <string>

GLShader::GLShader(const char* file) noexcept
    : GLShader(GLShaderTypeFromFileName(file), readShaderFile(file).c_str(), file) {
}

GLShader::GLShader(GLenum type, const char* source,const char* debugFileName) noexcept
    : type_(type)
    , handle_(glCreateShader(type)) {
    glShaderSource(handle_, 1, &source, nullptr);
    glCompileShader(handle_);

    char buffer[8912] = { 0 };
    GLsizei length = 0;
    glGetShaderInfoLog(handle_, sizeof(buffer), &length, buffer);

    if (length) {
        printf("%s, (file=%s)\n", buffer, debugFileName);
        printShaderSource(source);
        assert(false);
    }
}

GLShader::~GLShader() {
    glDeleteShader(handle_);
    type_ = 0;
}

GLenum GLShaderTypeFromFileName(const char* file) {
    if (endsWith(file, ".vert")) {
        return GL_VERTEX_SHADER;
    }
    else if (endsWith(file, ".geom")) {
        return GL_GEOMETRY_SHADER;
    }
    else if (endsWith(file, ".frag")) {
        return GL_FRAGMENT_SHADER;
    }
    else if (endsWith(file, ".tesc")) {
        return GL_TESS_CONTROL_SHADER;
    }
    else if (endsWith(file, ".tese")) {
        return GL_TESS_EVALUATION_SHADER;
    }
    else if (endsWith(file, ".comp")) {
        return GL_COMPUTE_SHADER;
    }

    assert(false);
    return 0;
}

void printProgramInfoLog(GLuint handle) {
    char buffer[8912] = { 0 };
    GLsizei length = 0;

    glGetProgramInfoLog(handle, sizeof(buffer), &length, buffer);
    if (length) {
        printf("%s\n", buffer);
        assert(false);
    }
}

GLProgram::GLProgram(const GLShader& shader)
    : handle_(glCreateProgram()) {
    glAttachShader(handle_, shader.getHandle());
    glLinkProgram(handle_);
    printProgramInfoLog(handle_);
}

GLProgram::GLProgram(const GLShader& shader, const GLShader& shader1) 
    : handle_(glCreateProgram()) {
    glAttachShader(handle_, shader.getHandle());
    glAttachShader(handle_, shader1.getHandle());
    glLinkProgram(handle_);
    printProgramInfoLog(handle_);
}

GLProgram::GLProgram(const GLShader& shader, const GLShader& shader1, const GLShader& shader2) 
    : handle_(glCreateProgram()) {
    glAttachShader(handle_, shader.getHandle());
    glAttachShader(handle_, shader1.getHandle());
    glAttachShader(handle_, shader2.getHandle());
    glLinkProgram(handle_);
    printProgramInfoLog(handle_);
}

GLProgram::GLProgram(const GLShader& shader, const GLShader& shader1, const GLShader& shader2,
    const GLShader& shader3) 
    : handle_(glCreateProgram()) {
    glAttachShader(handle_, shader.getHandle());
    glAttachShader(handle_, shader1.getHandle());
    glAttachShader(handle_, shader2.getHandle());
    glAttachShader(handle_, shader3.getHandle());
    glLinkProgram(handle_);
    printProgramInfoLog(handle_);
}

GLProgram::GLProgram(const GLShader& shader, const GLShader& shader1, const GLShader& shader2,
    const GLShader& shader3, const GLShader& shader4) 
    : handle_(glCreateProgram()) {
    glAttachShader(handle_, shader.getHandle());
    glAttachShader(handle_, shader1.getHandle());
    glAttachShader(handle_, shader2.getHandle());
    glAttachShader(handle_, shader3.getHandle());
    glAttachShader(handle_, shader4.getHandle());
    glLinkProgram(handle_);
    printProgramInfoLog(handle_);
}

GLProgram::~GLProgram() {
    glDeleteProgram(handle_);
}

void GLProgram::useProgram() {
    glUseProgram(handle_);
}

GLBuffer::GLBuffer(GLsizeiptr size, const void* data, GLbitfield flags) noexcept {
    glCreateBuffers(1, &handle_);
    glNamedBufferStorage(handle_, size, data, flags);
}

GLBuffer::~GLBuffer() {
    glDeleteBuffers(1, &handle_);
}
