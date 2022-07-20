#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include <glad/gl.h>

#include <glfw/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/version.h>
#include <meshoptimizer.h>

static const char* shaderCodeVertex = R"(
#version 460 core
layout(std140, binding = 0) uniform PerFrameData
{
	uniform mat4 MVP;
};
layout (location=0) in vec3 pos;
layout (location=0) out vec3 color;
void main()
{
	gl_Position = MVP * vec4(pos, 1.0);
	color = pos.xyz;
}
)";

static const char* shaderCodeGeometry = R"(
#version 460 core

layout( triangles ) in;
layout( triangle_strip, max_vertices = 3 ) out;

layout (location=0) in vec3 color[];
layout (location=0) out vec3 colors;
layout (location=1) out vec3 barycoords;

void main()
{
	const vec3 bc[3] = vec3[]
	(
		vec3(1.0, 0.0, 0.0),
		vec3(0.0, 1.0, 0.0),
		vec3(0.0, 0.0, 1.0)
	);
	for ( int i = 0; i < 3; i++ )
	{
		gl_Position = gl_in[i].gl_Position;
		colors = color[i];
		barycoords = bc[i];
		EmitVertex();
	}
	EndPrimitive();
}
)";

static const char* shaderCodeFragment = R"(
#version 460 core
layout (location=0) in vec3 colors;
layout (location=1) in vec3 barycoords;
layout (location=0) out vec4 out_FragColor;
float edgeFactor(float thickness)
{
	vec3 a3 = smoothstep( vec3( 0.0 ), fwidth(barycoords) * thickness, barycoords);
	return min( min( a3.x, a3.y ), a3.z );
}
void main()
{
	out_FragColor = vec4( mix( vec3(0.0), colors, edgeFactor(1.0) ), 1.0 );
};
)";

struct PerFrameData {
	glm::mat4 mvp;
};

int main() {
	glfwSetErrorCallback(
		[](int error, const char* desc) {
			fprintf(stderr, "Error: %s", desc);
		}
	);

	if (!glfwInit()) {
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "simple example", nullptr, nullptr);
	if (nullptr == window) {
		glfwTerminate();
		return -1;
	}
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

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &shaderCodeVertex, nullptr);
	glCompileShader(vertexShader);

	const GLuint shaderGeometry = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(shaderGeometry, 1, &shaderCodeGeometry, nullptr);
	glCompileShader(shaderGeometry);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &shaderCodeFragment, nullptr);
	glCompileShader(fragmentShader);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, shaderGeometry);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glUseProgram(program);

	const aiScene* scene = aiImportFile("data/rubber_duck/scene.gltf", aiPostProcessSteps::aiProcess_Triangulate);
	if (nullptr == scene || !scene->HasMeshes()) {
		glfwTerminate();
		return -1;
	}

	const aiMesh* mesh = scene->mMeshes[0];
	std::vector<glm::vec3> positions;
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		const aiVector3D& v = mesh->mVertices[i];
		positions.emplace_back(glm::vec3(v.x, v.z, v.y));
	}

	std::vector<unsigned int> indices;
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		const aiFace& face = mesh->mFaces[i];
		for (int j = 0; j < 3; ++j) {
			indices.push_back(face.mIndices[j]);
		}
	}
	aiReleaseImport(scene);

	std::vector<unsigned int> indicesLod;
	{
		std::vector<unsigned int> remap(indices.size());
		const size_t vertexCount = meshopt_generateVertexRemap(remap.data(), indices.data(),
		indices.size(), positions.data(), positions.size(), sizeof(glm::vec3));

		std::vector<unsigned int> remappedIndices(indices.size());;
		std::vector<glm::vec3> remappedVertices(vertexCount);
		meshopt_remapIndexBuffer(remappedIndices.data(), indices.data(), indices.size(), remap.data());
		meshopt_remapVertexBuffer(remappedVertices.data(), positions.data(), positions.size(),
			sizeof(glm::vec3), remap.data());

		meshopt_optimizeVertexCache(remappedIndices.data(), remappedIndices.data(), indices.size(), vertexCount);
		meshopt_optimizeOverdraw(remappedIndices.data(), remappedIndices.data(), indices.size(),
			glm::value_ptr(remappedVertices[0]), vertexCount, sizeof(glm::vec3), 1.05f);
		meshopt_optimizeVertexFetch(remappedVertices.data(), remappedIndices.data(), indices.size(),
			remappedVertices.data(), vertexCount, sizeof(glm::vec3));

		const float threshold = 0.2f;
		const size_t target_index_count = size_t(remappedIndices.size() * threshold);
		const float target_error = 1e-2f;

		indicesLod.resize(remappedIndices.size());
		indicesLod.resize(meshopt_simplify(&indicesLod[0], remappedIndices.data(),
			remappedIndices.size(), &remappedVertices[0].x, vertexCount, sizeof(glm::vec3),
			target_index_count, target_error));

		indices = remappedIndices;
		positions = remappedVertices;
	}
	
	const size_t sizeIndices = sizeof(unsigned int) * indices.size();
	const size_t sizeIndicesLod = sizeof(unsigned int) * indicesLod.size();
	const size_t sizeVertices = sizeof(glm::vec3) * positions.size();

	GLuint meshData;
	glCreateBuffers(1, &meshData);
	glNamedBufferStorage(meshData, sizeIndices + sizeIndicesLod + sizeVertices, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferSubData(meshData, 0, sizeIndices, indices.data());
	glNamedBufferSubData(meshData, sizeIndices, sizeIndicesLod, indicesLod.data());
	glNamedBufferSubData(meshData, sizeIndices + sizeIndicesLod, sizeVertices, positions.data());

	GLuint vao;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);

	glVertexArrayElementBuffer(vao, meshData);
	glVertexArrayVertexBuffer(vao, 0, meshData, sizeIndices + sizeIndicesLod, sizeof(glm::vec3));
	glEnableVertexArrayAttrib(vao, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, 0, 0);

	const size_t kBufferSize = sizeof(PerFrameData);

	GLuint perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	glNamedBufferStorage(perFrameDataBuffer, kBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kBufferSize);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window)) {
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		const float ratio = static_cast<float>(width) / height;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const glm::mat4 m1 = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, -0.5f, -1.5f)),
			(float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::mat4 m2 = glm::rotate(glm::translate(glm::mat4(1.0f),
			glm::vec3(+0.5f, -0.5f, -1.5f)), (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

		PerFrameData perFrameData{ p * m1 };
		glNamedBufferSubData(perFrameDataBuffer, 0, kBufferSize, &perFrameData);
		glDrawElements(GL_TRIANGLES, static_cast<unsigned>(indices.size()), GL_UNSIGNED_INT, nullptr);

		PerFrameData perFrameData2{ p * m2 };
		glNamedBufferSubData(perFrameDataBuffer, 0, kBufferSize, &perFrameData2);
		glDrawElements(GL_TRIANGLES, static_cast<unsigned>(indicesLod.size()), GL_UNSIGNED_INT, (void*)sizeIndices);

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
