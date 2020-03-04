#include "srrg_imgui-node-editor_app.h"
#include "imgui_impl_glfw_gl3.h"
#include <GL/gl3w.h> // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <imgui.h>
#include <srrg_system_utils/system_utils.h>
#include <stdio.h>
#include <vector>

static void error_callback(int error, const char* description) {
  fprintf(stderr, "Error %d: %s\n", error, description);
}

int main(int argc, char** argv) {
  srrg2_core::srrgInit(argc, argv, "srrg2_config_visualizer");

  // Setup window
  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
    return 1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window = glfwCreateWindow(1280, 720, srrg2_ine_Application_GetName(), NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync
  gl3wInit();

  ImGui::CreateContext();

  // Setup ImGui binding
  ImGui_ImplGlfwGL3_Init(window, true);

  ImGuiIO& io = ImGui::GetIO();

  io.MouseDrawCursor = false;

  ImVec4 clear_color = ImVec4(0.125f, 0.125f, 0.125f, 1.00f);

  srrg2_ine_Application_Initialize();

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplGlfwGL3_NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Content",
                 nullptr,
                 ImVec2(0, 0),
                 0.0f,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                   ImGuiWindowFlags_MenuBar);

    srrg2_ine_Application_Frame();

    ImGui::End();

    // Rendering
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    glfwSwapBuffers(window);
  }

  srrg2_ine_Application_Finalize();

  // Cleanup
  ImGui_ImplGlfwGL3_Shutdown();

  ImGui::DestroyContext();

  glfwTerminate();

  return 0;
}
