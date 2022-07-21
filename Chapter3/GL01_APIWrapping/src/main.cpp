#include <assert.h>
#include <stdio.h>

#include "GL.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>


static const char* shaderCodeVertex = R"(
#version 460 core
layout(std140, binding = 0) uniform PerFrameData
{
	uniform mat4 MVP;
};
layout (location=0) out vec2 uv;
const vec2 pos[3] = vec2[3](
	vec2(-0.6f, -0.4f),
	vec2( 0.6f, -0.4f),
	vec2( 0.0f,  0.6f)
);
const vec2 tc[3] = vec2[3](
	vec2( 0.0, 0.0 ),
	vec2( 1.0, 0.0 ),
	vec2( 0.5, 1.0 )
);
void main()
{
	gl_Position = MVP * vec4(pos[gl_VertexID], 0.0, 1.0);
	uv = tc[gl_VertexID];
}
)";

static const char* shaderCodeFragment = R"(
#version 460 core
layout (location=0) in vec2 uv;
layout (location=0) out vec4 out_FragColor;
uniform sampler2D texture0;
void main()
{
	out_FragColor = texture(texture0, uv);
};
)";

int main()
{
	glfwSetErrorCallback([](int error, const char* desc) {
		fprintf(stderr, "error: %s", desc);
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
	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scan, int action, int mod) {
		if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
		}
	);

	glfwMakeContextCurrent(window);
	
	GL4API api;
	GetAPI4(&api, [](const char* func) -> void* {
		return (void*)glfwGetProcAddress(func);
		});
	InjectAPITracer4(&api);

	GLuint shaderVertex = api.glCreateShader(GL_VERTEX_SHADER);
	api.glShaderSource(shaderVertex, 1, &shaderCodeVertex, nullptr);
	api.glCompileShader(shaderVertex);

	GLuint shaderFragment = api.glCreateShader(GL_FRAGMENT_SHADER);
	api.glShaderSource(shaderFragment, 1, &shaderCodeFragment, nullptr);
	api.glCompileShader(shaderFragment);

	GLuint program = api.glCreateProgram();
	api.glAttachShader(program, shaderVertex);
	api.glAttachShader(program, shaderFragment);
	api.glLinkProgram(program);

	GLuint vao;
	api.glCreateVertexArrays(1, &vao);
	api.glBindVertexArray(vao);

	const GLsizeiptr kBufferSize = sizeof(glm::mat4);

	GLuint perFrameDataBuffer;
	api.glCreateBuffers(1, &perFrameDataBuffer);
	api.glNamedBufferStorage(perFrameDataBuffer, kBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	api.glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kBufferSize);

	int w, h, comp;
	const uint8_t* img = stbi_load("data/ch2_sample3_STB.jpg", &w, &h, &comp, 3);

	GLuint texture;
	api.glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	api.glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
	api.glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	api.glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	api.glTextureStorage2D(texture, 1, GL_RGB8, w, h);
	api.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	api.glTextureSubImage2D(texture, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img);
	stbi_image_free((void*)img);
	api.glBindTextures(0, 1, &texture);

	api.glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	while (!glfwWindowShouldClose(window)) {
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		const float ratio = static_cast<float>(width) / height;

		api.glViewport(0, 0, width, height);
		api.glClear(GL_COLOR_BUFFER_BIT);
		const glm::mat4 m = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(), glm::vec3(0.0f, 0.0f, 1.0f));
		const glm::mat4 p = glm::ortho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		const glm::mat4 mvp = p * m;

		api.glUseProgram(program);
		api.glNamedBufferSubData(perFrameDataBuffer, 0, kBufferSize, glm::value_ptr(mvp));

		api.glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	api.glDeleteVertexArrays(1, &vao);
	api.glDeleteBuffers(1, &perFrameDataBuffer);
	api.glDeleteProgram(program);
	api.glDeleteShader(shaderFragment);
	api.glDeleteShader(shaderVertex);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
