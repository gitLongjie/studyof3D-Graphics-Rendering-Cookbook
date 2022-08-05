#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "shared/glFramework/GLShader.h"
#include "shared/UtilsMath.h"
#include "shared/Bitmap.h"
#include "shared/debug.h"
#include "shared/UtilsCubemap.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

struct PerFrameData {
    glm::mat4 model;
    glm::mat4 mvp;
    glm::vec4 cameraPos;
};

class Context {
public:
    Context() = default;
    ~Context() {
        glfwTerminate();
    }

    bool Init() const {
        return 0 != glfwInit();
    }
};

template <typename Func> 
struct LocalVar {
    LocalVar(Func func) : func_(func){}
    ~LocalVar() {
        func_();
    }
    Func func_ = nullptr;
};

int main() {
    glfwSetErrorCallback([](int error, const char* desc) {
        fprintf(stderr, "error: %d, desc:%s", error, desc);
        }
    );

    Context context;
    if (!context.Init()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1027, 768, "simple example", nullptr, nullptr);
    if (nullptr == window) {
        return -1;
    }
    auto DestoryWindowFunc = [&window]() {
        if (nullptr != window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }
    };
    LocalVar<decltype(DestoryWindowFunc)> destoryWindow(DestoryWindowFunc);

    glfwSetKeyCallback(window,
        [](GLFWwindow* window, int key, int scan, int action, int mod) {
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
        printf("Unable to load data/rubber_duck/scene.gltf\n");
        return -1;
    }

    struct VertexData {
        glm::vec3 pos;
        glm::vec3 nor;
        glm::vec2 tc;
    };

    std::vector<VertexData> vertices;
    const aiMesh* mesh = scene->mMeshes[0];
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        const aiVector3D& vertice = mesh->mVertices[i];
        const aiVector3D& normal = mesh->mNormals[i];
        const aiVector3D& tc = mesh->mTextureCoords[0][i];
        VertexData vertexData{ { vertice.x, vertice.z, vertice.y },
            {normal.x, normal.z, normal.y}, {tc.x, tc.y } };
        vertices.emplace_back(std::move(vertexData));
    }

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (int f = 0; f < 3; ++f) {
            indices.emplace_back(face.mIndices[f]);
        }
    }
    aiReleaseImport(scene);

    const size_t kSizeIndices = sizeof(unsigned int) * indices.size();
    const size_t kSizeVertices = sizeof(VertexData) * vertices.size();

    GLuint dataIndices;
    glCreateBuffers(1, &dataIndices);
    glNamedBufferStorage(dataIndices, kSizeIndices, indices.data(), 0);

    GLuint vao;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexArrayElementBuffer(vao, dataIndices);

    GLuint dataVertices;
    glCreateBuffers(1, &dataVertices);
    glNamedBufferStorage(dataVertices, kSizeVertices, vertices.data(), 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, dataVertices);

    GLShader modelVertextShader("data/shaders/chapter03/GL03_duck.vert");
    GLShader modelFragmentShader("data/shaders/chapter03/GL03_duck.frag");
    GLProgram progModel(modelVertextShader, modelFragmentShader);

    GLShader cubeVertextShader("data/shaders/chapter03/GL03_cube.vert");
    GLShader cubeFragmentShader("data/shaders/chapter03/GL03_cube.frag");
    GLProgram progCube(cubeVertextShader, cubeFragmentShader);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLuint texture;
    {
        int w, h, comp;
        const uint8_t* img = stbi_load("data/rubber_duck/textures/Duck_baseColor.png", &w, &h, &comp, 3);
        if (nullptr == img) {
            printf("duck base color is nullptr ");
            return -1;
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &texture);
        glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
        glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(texture, 1, GL_RGB8, w, h);
        glTextureSubImage2D(texture, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img);
        glBindTextures(0, 1, &texture);

        stbi_image_free((void*)img);
    }

    GLuint cubemapTex;
    {
        int w, h, comp;
        const float* img = stbi_loadf("data/piazza_bologni_1k.hdr", &w, &h, &comp, 3);
        Bitmap in(w, h, comp, eBitmapFormat_Float, img);
        Bitmap out = convertEquirectangularMapToVerticalCross(in);
        stbi_image_free((void*)img);

        stbi_write_hdr("screenshot.hdr", out.w_, out.h_, out.comp_, (const float*)out.data_.data());

        Bitmap cubemap = convertVerticalCrossToCubeMapFaces(out);

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cubemapTex);
        glTextureParameteri(cubemapTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(cubemapTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(cubemapTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(cubemapTex, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(cubemapTex, GL_TEXTURE_MAX_LEVEL, 0);
        glTextureParameteri(cubemapTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(cubemapTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(cubemapTex, 1, GL_RGB32F, cubemap.w_, cubemap.h_);

        const uint8_t* data = cubemap.data_.data();
        for (unsigned int i = 0; i < 6; ++i) {
            glTextureSubImage3D(cubemapTex, 0, 0, 0, i, cubemap.w_,
                cubemap.h_, 1, GL_RGB, GL_FLOAT, data);
            data += cubemap.w_ * cubemap.h_ * cubemap.comp_
                * Bitmap::getBytesPerComponent(cubemap.fmt_);
        }
        glBindTextures(1, 1, &cubemapTex);
    }

    

    const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

    GLuint perFrameDataBuffer;
    glCreateBuffers(1, &perFrameDataBuffer);
    glNamedBufferStorage(perFrameDataBuffer, kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kUniformBufferSize);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        float ratio = static_cast<float>(width) / height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

        {
            const glm::mat4 m = glm::rotate(glm::translate(glm::mat4(1.0f), vec3(0.0f, -0.5f, -1.5f)),
                (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
            const PerFrameData perFrameData = { .model = m, .mvp = p * m, .cameraPos = glm::vec4(0.0f)};
            progModel.useProgram();
            glNamedBufferSubData(perFrameDataBuffer, 0, kUniformBufferSize, &perFrameData);
            glDrawElements(GL_TRIANGLES, static_cast<unsigned>(indices.size()), GL_UNSIGNED_INT, nullptr);
        }

        {
            const glm::mat4 m = glm::scale(glm::mat4(1.0f), vec3(2.0f));
            const PerFrameData perFrameData = { .model = m, .mvp = p * m, .cameraPos = vec4(0.0f) };
            progCube.useProgram();
            glNamedBufferSubData(perFrameDataBuffer, 0, kUniformBufferSize, &perFrameData);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &dataIndices);
    glDeleteBuffers(1, &dataVertices);
    glDeleteBuffers(1, &perFrameDataBuffer);
    glDeleteVertexArrays(1, &vao);

    return 0;
}
