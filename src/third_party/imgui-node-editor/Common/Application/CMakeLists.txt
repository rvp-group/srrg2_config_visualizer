
set(_Application_Sources
    Include/srrg_imgui-node-editor_app.h
)

find_package(glfw3 3 REQUIRED)
find_package(OpenGL REQUIRED)

list(APPEND _Application_Sources
    Source/GLFW/imgui_impl_glfw_gl3.cpp
    Source/GLFW/imgui_impl_glfw_gl3.h
    Source/GLFW/srrg_imgui-node-editor_app.cpp
)


source_group("" FILES ${_Application_Sources})
source_group("__Application__" FILES ${_Application_Sources})

add_library(Application SHARED ${_Application_Sources})

target_include_directories(Application PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)

find_package(imgui REQUIRED)
target_link_libraries(Application PUBLIC imgui)

target_include_directories(Application PRIVATE ${OPENGL_INCLUDE_DIR})

find_package(gl3w REQUIRED)
target_link_libraries(Application PRIVATE ${OPENGL_gl_LIBRARY} gl3w glfw)

set_property(TARGET Application PROPERTY FOLDER "Common")
