#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "shared/glFramework/GLShader.h"
#include "shared/debug.h"

#include <vector>

struct PerFrameData {
    glm::mat4 mvp;
};

int main() {
    glfwSetErrorCallback([](int error, const char* desc) {
        fprintf(stderr, "error= %d, desc=%s", error, desc);
        });

    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1024, 720, "simple example", nullptr, nullptr);
    if (nullptr == window) {
        glfwTerminate();
        return -1;
    }
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scan, int action, int mod) {
        if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        }
    );

    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);

    const aiScene* scene = aiImportFile("data/rubber_duck/scene.gltf", aiProcess_Triangulate);
    if (nullptr == scene || !scene->HasMeshes()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    const aiMesh* mesh = scene->mMeshes[0];
    struct VertexData {
        glm::vec3 pos;
        glm::vec2 tc;
    };

    std::vector<VertexData> vertices;
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        const aiVector3D& pos = mesh->mVertices[i];
        const aiVector3D& tc =mesh->mTextureCoords[0][i];
        vertices.push_back({ glm::vec3(pos.x, pos.z, pos.y), glm::vec2(tc.x, tc.y) });
    }

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }
    aiReleaseImport(scene);

    GLShader vertexShader("data/shaders/chapter03/GL02.vert");
    GLShader geomShader("data/shaders/chapter03/GL02.geom");
    GLShader fragShader("data/shaders/chapter03/GL02.frag");

    const size_t kSizeIndices = sizeof(unsigned int) * indices.size();
    const size_t kSizeVertices = sizeof(VertexData) * vertices.size();

    GLuint indicesBuffer;
    glCreateBuffers(1, &indicesBuffer);
    glNamedBufferStorage(indicesBuffer, kSizeIndices, indices.data(), 0);

    GLuint vao;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexArrayElementBuffer(vao, indicesBuffer);

    GLuint dataVertices;
    glCreateBuffers(1, &dataVertices);
    glNamedBufferStorage(dataVertices, kSizeVertices, vertices.data(), 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, dataVertices);

    int w, h, comp;
    const uint8_t* img = stbi_load("data/rubber_duck/textures/Duck_baseColor.png", &w, &h, &comp, 3);
    if (nullptr == img) {
        glDeleteVertexArrays(1, &dataVertices);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &indicesBuffer);
        glfwDestroyWindow(window);
        return -1;
    }

    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureStorage2D(texture, 1, GL_RGB8, w, h);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glTextureSubImage2D(texture, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img);
    glBindTextures(0, 1, &texture);

    stbi_image_free((void*)img);

    const GLsizei kPerFrameDataSize = sizeof(PerFrameData);

    GLuint perFrameDataBuffer;
    glCreateBuffers(1, &perFrameDataBuffer);
    glNamedBufferStorage(perFrameDataBuffer, kPerFrameDataSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kPerFrameDataSize);

    GLProgram program(vertexShader, geomShader, fragShader);
    program.useProgram();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    
    while (!glfwWindowShouldClose(window)) {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        float ratio = static_cast<float>(width) / height;

        glViewport(0, 0, width, height);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        glm::mat4 m = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, -1.5f)),
            (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

        PerFrameData perFrameData{ .mvp = p * m };
        glNamedBufferSubData(perFrameDataBuffer, 0, kPerFrameDataSize, (const void*)&perFrameData);

        glDrawElements(GL_TRIANGLES, static_cast<unsigned>(indices.size()), GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &dataVertices);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &indicesBuffer);
    glfwDestroyWindow(window);
    return 0;
}