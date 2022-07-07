cmake_minimum_required(VERSION 3.00)

project(main)
set (CMAKE_CXX_STANDARD 14)

FILE(GLOB SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/main.cc
  ${PROJECT_SOURCE_DIR}/glad.cc
  ${PROJECT_SOURCE_DIR}/stab_image.cc)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

include_directories(${PROJECT_SOURCE_DIR}/deps)

target_link_libraries(${PROJECT_NAME} space)

add_subdirectory(test)
add_subdirectory(src)
