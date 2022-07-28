#pragma once

#include <glad/gl.h>

class GLShader {
public:
    explicit GLShader(const char* file) noexcept;
    explicit GLShader(GLenum type, const char* source, const char* debugFileName=nullptr) noexcept;
    ~GLShader();

    GLenum getType() const { return type_; }
    GLuint getHandle() const { return handle_; }
private:
    GLenum type_;
    GLuint handle_;
};

class GLProgram {
public:
    explicit GLProgram(const GLShader& shader);
    explicit GLProgram(const GLShader& shader, const GLShader& shader1);
    explicit GLProgram(const GLShader& shader, const GLShader& shader1, const GLShader& shader2);
    explicit GLProgram(const GLShader& shader, const GLShader& shader1, const GLShader& shader2,
        const GLShader& shader3);
    explicit GLProgram(const GLShader& shader, const GLShader& shader1, const GLShader& shader2,
        const GLShader& shader3, const GLShader& shader4);
    ~GLProgram();

    void useProgram();
    GLuint getHandle() const { return handle_; }
private:
    GLuint handle_;
};

class GLBuffer {
public:
    explicit GLBuffer(GLsizeiptr size, const void* data, GLbitfield flags) noexcept;
    ~GLBuffer();

private:
    GLuint handle_;
};
GLenum GLShaderTypeFromFileName(const char* file);
