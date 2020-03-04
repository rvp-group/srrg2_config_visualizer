add_library(gl3w SHARED
    Include/GL/gl3w.h
    Include/GL/glcorearb.h
    Source/gl3w.c
)


if (UNIX AND (NOT APPLE))
    # Linux: GL is required to pull glXGetProcAddress
    target_link_libraries(gl3w PRIVATE GL dl)
endif()

target_include_directories(gl3w PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)

