cmake_minimum_required(VERSION 3.5)
project(ImGuiNodeEditor)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Point CMake where to look for module files.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

set(CMAKE_CXX_STANDARD            14)
set(CMAKE_CXX_STANDARD_REQUIRED   YES)

add_subdirectory(Common/Application)
add_subdirectory(Common/BlueprintUtilities)
