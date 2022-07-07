cmake_minimum_required(VERSION 3.00)

project(space)
set (CMAKE_CXX_STANDARD 14)

FILE(GLOB SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/core/*.cc
  ${PROJECT_SOURCE_DIR}/errors/*.cc
  ${PROJECT_SOURCE_DIR}/loaders/*.cc
  ${PROJECT_SOURCE_DIR}/renders/*.cc)

add_library(${PROJECT_NAME} ${SOURCE_FILES})

include_directories(../deps)

find_library(ASSIMP_LIB assimp REQUIRED)
target_link_libraries(${PROJECT_NAME} ${ASSIMP_LIB})

find_package(glfw3 REQUIRED)
target_link_libraries(${PROJECT_NAME} glfw)

find_package(OpenGL REQUIRED)
target_link_libraries (${PROJECT_NAME} OpenGL::GL)

find_package (Glog REQUIRED)
target_link_libraries (${PROJECT_NAME} glog::glog)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)
