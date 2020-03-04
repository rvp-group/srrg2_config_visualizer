add_library(srrg_config_visualizer_library SHARED
  config_node.cpp config_node.h
  configurable_node_manager.cpp configurable_node_manager.h
)

target_link_libraries(srrg_config_visualizer_library
  imgui_node_editor Application blueprint-utilities 
)

target_include_directories(srrg_config_visualizer_library PUBLIC
${IMGUI_NODE_EDITOR_ROOT}/NodeEditor/Source
${IMGUI_NODE_EDITOR_ROOT}/NodeEditor/Include
)

