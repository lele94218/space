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

find_library(ASSIMP_LIB assimp REQUIRED)
target_link_libraries(space ${ASSIMP_LIB})

find_package(glfw3 REQUIRED)
target_link_libraries(space glfw)

find_package(OpenGL REQUIRED)
target_link_libraries (space OpenGL::GL)

find_package (Glog REQUIRED)
target_link_libraries (space glog::glog)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
