cmake_minimum_required(VERSION 3.22)

project(space)
set (CMAKE_CXX_STANDARD 14)
set(EXECUTABLE_OUTPUT_PATH "build")

FILE(GLOB SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/*.cc
  ${PROJECT_SOURCE_DIR}/src/core/*.cc
  ${PROJECT_SOURCE_DIR}/src/loaders/*.cc)

add_executable(space ${SOURCE_FILES})

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories("/opt/homebrew/include")

find_library(GLOG_LIB glog REQUIRED)
find_library(GLFW_LIB glfw REQUIRED)
find_library(ASSIMP_LIB assimp REQUIRED)

target_link_libraries(space ${GLOG_LIB})
target_link_libraries(space ${GLFW_LIB})
target_link_libraries(space ${ASSIMP_LIB})
