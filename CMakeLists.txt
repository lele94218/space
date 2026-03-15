cmake_minimum_required(VERSION 3.5)

project(main)
set (CMAKE_CXX_STANDARD 14)

# ImGui sources
set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/deps/imgui)
set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

FILE(GLOB SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/main.cc
  ${PROJECT_SOURCE_DIR}/glad.cc
  ${PROJECT_SOURCE_DIR}/stab_image.cc)

include_directories(deps)
include_directories(${IMGUI_DIR})
include_directories(${IMGUI_DIR}/backends)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${IMGUI_SOURCES})

include_directories("/opt/homebrew/include/SDL2")
find_library(SDL2_LIBRARY SDL2)
if(NOT SDL2_LIBRARY)
    message(FATAL_ERROR "SDL2 library not found")
endif()
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY})

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

target_link_libraries(${PROJECT_NAME} space)

add_subdirectory(test)
add_subdirectory(src)
