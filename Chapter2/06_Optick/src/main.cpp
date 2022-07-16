#include <stdio.h>
#include <stdlib.h>

#include <glad/gl.h>
#include <glfw/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <optick.h>

static const char* shaderCodeVertex = R"(
#version 460 core
layout(std140, binding = 0) uniform PerFrameData
{
	uniform mat4 MVP;
	uniform int isWireframe;
};
layout (location=0) out vec3 color;
const vec3 pos[8] = vec3[8](
	vec3(-1.0f,-1.0f, 1.0f),
	vec3( 1.0f,-1.0f, 1.0f),
	vec3( 1.0f, 1.0f, 1.0f),
	vec3(-1.0f, 1.0f, 1.0f),

	vec3(-1.0f,-1.0f,-1.0f),
	vec3( 1.0f,-1.0f,-1.0f),
	vec3( 1.0f, 1.0f,-1.0f),
	vec3(-1.0f, 1.0f,-1.0f)
);
const vec3 col[8] = vec3[8](
	vec3( 1.0f, 0.0f, 0.0f),
	vec3( 0.0f, 1.0f, 0.0f),
	vec3( 0.0f, 0.0f, 1.0f),
	vec3( 1.0f, 1.0f, 0.0f),

	vec3( 1.0f, 1.0f, 0.0f),
	vec3( 0.0f, 0.0f, 1.0f),
	vec3( 0.0f, 1.0f, 0.0f),
	vec3( 1.0f, 0.0f, 0.0f)
);
const int indices[36] = int[36](
	// front
	0, 1, 2, 2, 3, 0,
	// right
	1, 5, 6, 6, 2, 1,
	// back
	7, 6, 5, 5, 4, 7,
	// left
	4, 0, 3, 3, 7, 4,
	// bottom
	4, 5, 1, 1, 0, 4,
	// top
	3, 2, 6, 6, 7, 3
);
void main()
{
	int idx = indices[gl_VertexID];
	gl_Position = MVP * vec4(pos[idx], 1.0);
	color = isWireframe > 0 ? vec3( 0.0f ) : col[idx];
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
		[](int error, const char* desc) {
			fprintf(stderr, "Error: %s", desc);
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

	const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &shaderCodeVertex, nullptr);
	glCompileShader(vertexShader);

	const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &shaderCodeFragment, nullptr);
	glCompileShader(fragmentShader);

	const GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLuint vao;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLsizei perFrameDataSize = sizeof(PerFrameData);

	GLuint perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	glNamedBufferStorage(perFrameDataBuffer, perFrameDataSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, perFrameDataSize);

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
		glm::mat4 p = glm::perspective(45.0f, rate, 0.1f, 1000.0f);

		glm::mat4 m = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.5f)),
			static_cast<float>(glfwGetTime()), glm::vec3(1.0f, 1.0f, 1.0f));
		PerFrameData perFrameData{ p * m, false };

		glUseProgram(program);
		{
			glNamedBufferSubData(perFrameDataBuffer, 0, perFrameDataSize, &perFrameData);

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

        {
            glNamedBufferSubData(perFrameDataBuffer, 0, perFrameDataSize, &perFrameData);

			perFrameData.isWireframe = true;
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteBuffers(1, &perFrameDataBuffer);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	glfwDestroyWindow(window);
	glfwTerminate();

    return 0;
}