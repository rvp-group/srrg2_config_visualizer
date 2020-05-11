cmake_minimum_required(VERSION 2.8.3)
project(srrg2_config_visualizer)

find_package(catkin REQUIRED COMPONENTS
  srrg_cmake_modules
  sensor_msgs
  srrg2_core
  )

include(${srrg_cmake_modules_INCLUDE_DIRS}/CMakeCompileOptions.txt)
# imgui does not like pedoantic
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=pedantic")

set(IMGUI_NODE_EDITOR_ROOT ${CMAKE_SOURCE_DIR}/src/third_party/imgui-node-editor)

list(APPEND CMAKE_MODULE_PATH ${IMGUI_NODE_EDITOR_ROOT}/CMakeModules)

find_package(OpenGL REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(imgui REQUIRED)
find_package(glfw3 3 REQUIRED)
find_package(imgui_node_editor REQUIRED)

catkin_package(
  CATKIN_DEPENDS
  srrg_cmake_modules
  sensor_msgs
  srrg2_core
)

include_directories(src)

include_directories(
  ${OPENGL_INCLUDE_DIR}
  ${EIGEN3_INCLUDE_DIR}
  ${catkin_INCLUDE_DIRS}
)

link_directories(${catkin_LIBRARY_DIRS})

add_subdirectory(src)
