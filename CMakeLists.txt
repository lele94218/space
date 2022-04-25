cmake_minimum_required(VERSION 3.22)

project(space)
set (CMAKE_CXX_STANDARD 11)
set(EXECUTABLE_OUTPUT_PATH "build")

FILE(GLOB SOURCE_FILES *.cc)

add_executable(space ${SOURCE_FILES})

include_directories(include)
include_directories("/opt/homebrew/include")

find_library(GLOG_LIB glog REQUIRED)
find_library(GLFW_LIB glfw REQUIRED)
find_library(ASSIMP_LIB assimp REQUIRED)

target_link_libraries(space ${GLOG_LIB})
target_link_libraries(space ${GLFW_LIB})
target_link_libraries(space ${ASSIMP_LIB})
