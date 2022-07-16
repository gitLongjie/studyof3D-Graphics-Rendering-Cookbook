#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include <glad/gl.h>
#include <glfw/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

static const char* shaderCodeVertex = R"(
#version 460 core
layout(std140, binding = 0) uniform PerFrameData
{
	uniform mat4 MVP;
	uniform int isWireframe;
};
layout (location=0) in vec3 pos;
layout (location=0) out vec3 color;
void main()
{
	gl_Position = MVP * vec4(pos, 1.0);
	color = isWireframe > 0 ? vec3(0.0f) : pos.xyz;
}
)";

static const char* shaderCodeFragment = R"(
#version 460 core
layout (location=0) in vec3 color;
layout (location=0) out vec4 out_FragColor;
void main()
{
	out_FragColor = vec4(color, 1.0);
};
)";

struct PerFrameData {
    glm::mat4 mvp;
    int isWireframe;
};


int main() {
	glfwSetErrorCallback(
		[](int error, const char* des) {
			fprintf(stderr, "Error: %s", des);
		}
	);

	if (!glfwInit()) {
		exit(EXIT_FAILURE);
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "simple example", nullptr, nullptr);
	if (nullptr == window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
		return -1;
	}
	glfwSetKeyCallback(window, 
		[](GLFWwindow* window, int key, int scane, int action, int mods) {
			if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			}
		}
	);

	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(1);

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &shaderCodeVertex, nullptr);
	glCompileShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &shaderCodeFragment, nullptr);
	glCompileShader(fragmentShader);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLuint vao;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint meshData;
	glCreateBuffers(1, &meshData);

	const aiScene* scene = aiImportFile("data/rubber_duck/scene.gltf", aiProcess_Triangulate);
	if (nullptr == scene || !scene->HasMeshes()) {
        printf("Unable to load data/rubber_duck/scene.gltf\n");
		glfwTerminate();
        exit(255);
		return -1;
	}

	const aiMesh* mesh = scene->mMeshes[0];
	std::vector<glm::vec3> position;
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		const aiFace& face = mesh->mFaces[i];
		const unsigned int idx[3] = { face.mIndices[0], face.mIndices[1], face.mIndices[2] };
		for (int j = 0; j < 3; ++j) {
			const aiVector3D& v = mesh->mVertices[idx[j]];
			position.emplace_back(glm::vec3(v.x, v.z, v.y));
		}
	}
	aiReleaseImport(scene);

	glNamedBufferStorage(meshData, sizeof(glm::vec3) * position.size(), position.data(), 0);
	
	glVertexArrayVertexBuffer(vao, 0, meshData, 0, sizeof(glm::vec3));
	glEnableVertexArrayAttrib(vao, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, 0, 0);

	const int numVertices = static_cast<int>(position.size());

	const int perFrameDataBufferSize = sizeof(PerFrameData);
	GLuint perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	glNamedBufferStorage(perFrameDataBuffer, perFrameDataBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, perFrameDataBufferSize);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(-1.0f, -1.0f);

	while (!glfwWindowShouldClose(window)) {
		int with = 0;
		int height = 0;

		glfwGetFramebufferSize(window, &with, &height);
		float rate = static_cast<float>(with) / height;
		glViewport(0, 0, with, height);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		glm::mat4 p = glm::perspective(45.0f, rate, 0.1f, 1000.0f);

		glm::mat4 m = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, -1.5f)),
			static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));

		PerFrameData perFrameData = { p * m, false };
		glUseProgram(program);
		
		glNamedBufferSubData(perFrameDataBuffer, 0, perFrameDataBufferSize, &perFrameData);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawArrays(GL_TRIANGLES, 0, numVertices);

		perFrameData.isWireframe = true;
		glNamedBufferSubData(perFrameDataBuffer, 0, perFrameDataBufferSize, &perFrameData);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, numVertices);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

    glDeleteBuffers(1, &meshData);
    glDeleteBuffers(1, &perFrameDataBuffer);
    glDeleteProgram(program);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(window);
    glfwTerminate();

	return 0;
}