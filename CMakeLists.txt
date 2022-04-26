cmake_minimum_required(VERSION 3.00)

project(main)
set (CMAKE_CXX_STANDARD 14)

FILE(GLOB SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cc)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

include_directories(${PROJECT_SOURCE_DIR}/deps)

find_library(ASSIMP_LIB assimp REQUIRED)
target_link_libraries(${PROJECT_NAME} ${ASSIMP_LIB})

find_package(glfw3 REQUIRED)
target_link_libraries(${PROJECT_NAME} glfw)

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

find_package(Glog REQUIRED)
target_link_libraries(${PROJECT_NAME} glog::glog)

add_subdirectory(test)
add_subdirectory(src)
