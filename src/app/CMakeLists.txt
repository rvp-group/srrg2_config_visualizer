if (glfw3_FOUND) 
add_executable(app_node_editor app_node_editor.cpp)

find_package(imgui REQUIRED)
find_package(imgui_node_editor REQUIRED)
#find_package(blueprint-utilities REQUIRED)

target_include_directories(app_node_editor PUBLIC
${IMGUI_NODE_EDITOR_ROOT}/Common/Application/Include
${IMGUI_NODE_EDITOR_ROOT}/Common/BlueprintUtilities/Include
${IMGUI_NODE_EDITOR_ROOT}/ThirdParty/imgui
${IMGUI_NODE_EDITOR_ROOT}/ThirdParty/gl3w
${IMGUI_NODE_EDITOR_ROOT}/NodeEditor/Include
${IMGUI_NODE_EDITOR_ROOT}/NodeEditor/Source
)

target_link_libraries(app_node_editor PRIVATE imgui imgui_node_editor Application blueprint-utilities)
target_link_libraries(app_node_editor PUBLIC 
  srrg_config_visualizer_library
  ${GLUT_LIBRARIES}
  ${catkin_LIBRARIES}
  ${OPENGL_LIBRARIES})
endif()
  