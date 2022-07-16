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
	glfwWindowHint(GLFW_OPENGL_CORE_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &shaderCodeFragment, nullptr);
	glCompileShader(fragmentShader);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

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

	std::vector<unsigned int> indics;
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		const aiFace& face = mesh->mFaces[i];
		for (int j = 0; j < 3; ++j) {
			indics.push_back(face.mIndices[j]);
		}
	}
	aiReleaseImport(scene);

	std::vector<unsigned int> indicesLod;
	std::vector<unsigned int> remap(indics.size());

	const size_t vertexCount = meshopt_generateVertexRemap(remap.data(), indics.data(), indics.size(), positions.data(), positions.size(), sizeof(glm::vec3));

	std::vector<unsigned int> remapIndics(indics.size());;
	std::vector<glm::vec3> remappedVertices(vertexCount);
	meshopt_remapIndexBuffer(remapIndics.data(), indics.data(), indics.size(), remap.data());
	meshopt_remapVertexBuffer(remappedVertices.data(), positions.data(), positions.size(), sizeof(glm::vec3), remap.data());

	meshopt_optimizeVertexCache(remapIndics.data(), remapIndics.data(), indics.size(), vertexCount);
	meshopt_optimizeOverdraw

	

	GLuint meshData;
	glCreateBuffers(1, &meshData);
//	glNamedBufferData(meshData, positions.size(), &positions,  )


	GLuint vao;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);


	return 0;
}
