cmake_minimum_required(VERSION 3.00)

project(main)
set (CMAKE_CXX_STANDARD 14)

FILE(GLOB SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/main.cc
  ${PROJECT_SOURCE_DIR}/glad.cc
  ${PROJECT_SOURCE_DIR}/stab_image.cc)

include_directories(deps)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

include_directories("/opt/homebrew/include/SDL2")
find_library(SDL2_LIBRARY SDL2)
if(NOT SDL2_LIBRARY)
    message(FATAL_ERROR "SDL2 library not found")
endif()
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY})

target_link_libraries(${PROJECT_NAME} space)

add_subdirectory(test)
add_subdirectory(src)
