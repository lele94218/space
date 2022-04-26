cmake_minimum_required(VERSION 3.00)

project(space_test)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

FILE(GLOB TEST_FILES *.cc)

add_executable(space_test ${TEST_FILES})

target_link_libraries(space_test
  gtest_main
  space)

include(GoogleTest)
gtest_discover_tests(space_test)
