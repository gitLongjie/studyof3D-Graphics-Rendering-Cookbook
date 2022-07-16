#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

int main() {
	glfwSetErrorCallback([](int error, const char* reason) {
		fprintf(stderr, "Error:%s", reason);
		}
	);

	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1024, 768, "simple example", nullptr, nullptr);
	if (nullptr == window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scanmod, int action, int mod) {
        if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        }
    );

	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(1);

    const GLchar* vertexCodeShader = R"(
		#version 460 core
		layout (location = 0) in vec2 Position;
		layout (location = 1) in vec2 UV;
		layout (location = 2) in vec4 Color;
		layout(std140, binding = 0) uniform PerFrameData
		{
			uniform mat4 MVP;
		};
		out vec2 Frag_UV;
		out vec4 Frag_Color;
		void main()
		{
			Frag_UV = UV;
			Frag_Color = Color;
			gl_Position = MVP * vec4(Position.xy,0,1);
		}
	)";

    const GLchar* fragmetCodeShader = R"(
		#version 460 core
		in vec2 Frag_UV;
		in vec4 Frag_Color;
		layout (binding = 0) uniform sampler2D Texture;
		layout (location = 0) out vec4 Out_Color;
		void main()
		{
			Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
		}
	)";

	GLuint vao;
	glCreateVertexArrays(1, &vao);
	
	GLuint handleVBO;
	glCreateBuffers(1, &handleVBO);
	glNamedBufferStorage(handleVBO, 128 * 1024, nullptr, GL_DYNAMIC_STORAGE_BIT);

	GLuint handleElements;
	glCreateBuffers(1, &handleElements);
	glNamedBufferStorage(handleElements, 256 * 1024, nullptr, GL_DYNAMIC_STORAGE_BIT);

	glVertexArrayElementBuffer(vao, handleElements);
	glVertexArrayVertexBuffer(vao, 0, handleVBO, 0, sizeof(ImDrawVert));

	glEnableVertexArrayAttrib(vao, 0);
	glEnableVertexArrayAttrib(vao, 1);
	glEnableVertexArrayAttrib(vao, 2);

	glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, IM_OFFSETOF(ImDrawVert, pos));
	glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, IM_OFFSETOF(ImDrawVert, uv));
	glVertexArrayAttribFormat(vao, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, IM_OFFSETOF(ImDrawVert, col));

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribBinding(vao, 1, 0);
	glVertexArrayAttribBinding(vao, 2, 0);

	glBindVertexArray(vao);
	
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexCodeShader, nullptr);
	glCompileShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmetCodeShader, nullptr);
	glCompileShader(fragmentShader);

	GLuint progrom = glCreateProgram();
	glAttachShader(progrom, vertexShader);
	glAttachShader(progrom, fragmentShader);
	glLinkProgram(progrom);
	glUseProgram(progrom);

	GLuint preFrameDataBuffer;
	glCreateBuffers(1, &preFrameDataBuffer);
	glNamedBufferStorage(preFrameDataBuffer, sizeof(glm::mat4), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, preFrameDataBuffer);

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	ImFontConfig cfg = ImFontConfig();
    cfg.FontDataOwnedByAtlas = false;
    cfg.RasterizerMultiply = 1.5f;
    cfg.SizePixels = 768.0f / 32.0f;
    cfg.PixelSnapH = true;
    cfg.OversampleH = 4;
    cfg.OversampleV = 4;
    ImFont* Font = io.Fonts->AddFontFromFileTTF("data/OpenSans-Light.ttf", cfg.SizePixels, &cfg);

	unsigned char* pixels = nullptr;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureStorage2D(texture, 1, GL_RGBA8, width, height);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glPixelStorei(GL_PACK_ALIGNMENT, 0);
	glBindTextures(0, 1, &texture);

	io.Fonts->TexID = &texture;
	io.FontDefault = Font;
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
   
	while (!glfwWindowShouldClose(window)) {
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(width, height);
		ImGui::NewFrame();
		ImGui::ShowDemoWindow();
		ImGui::Render();

		const ImDrawData* draw_data = ImGui::GetDrawData();
        const float L = draw_data->DisplayPos.x;
        const float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        const float T = draw_data->DisplayPos.y;
        const float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        const glm::mat4 orthoProjection = glm::ortho(L, R, B, T);

		glNamedBufferSubData(preFrameDataBuffer, 0, sizeof(glm::mat4), glm::value_ptr(orthoProjection));

		for (int n = 0; n < draw_data->CmdListsCount; ++n) {
			const ImDrawList* drawList = draw_data->CmdLists[n];
			glNamedBufferSubData(handleVBO, 0, (GLsizei)drawList->VtxBuffer.Size * sizeof(ImDrawVert), drawList->VtxBuffer.Data);
			glNamedBufferSubData(handleElements, 0, (GLsizei)drawList->IdxBuffer.Size * sizeof(ImDrawIdx), drawList->IdxBuffer.Data);

            for (int cmd_i = 0; cmd_i < drawList->CmdBuffer.Size; cmd_i++) {
                const ImDrawCmd* pcmd = &drawList->CmdBuffer[cmd_i];
                const ImVec4 cr = pcmd->ClipRect;
                glScissor((int)cr.x, (int)(height - cr.w), (int)(cr.z - cr.x), (int)(cr.w - cr.y));
                glBindTextureUnit(0, (GLuint)(intptr_t)pcmd->TextureId);
                glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT,
                    (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)), (GLint)pcmd->VtxOffset);
            }
		}

        glScissor(0, 0, width, height);

        glfwSwapBuffers(window);
        glfwPollEvents();
	}

    ImGui::DestroyContext();

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
	return 0;
}
