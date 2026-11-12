cmake_minimum_required(VERSION 3.16)
project(ut_smock LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.15.2)
# fff is header-only: SOURCE_SUBDIR points at nothing so its own CMakeLists (and bundled old gtest) is not added
FetchContent_Declare(fff GIT_REPOSITORY https://github.com/meekrosoft/fff.git GIT_TAG v1.1 SOURCE_SUBDIR none)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest fff)

add_executable(${PROJECT_NAME} test_smoke.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest_main)
# fff's repo root also holds an old gtest/ copy, so expose only fff.h
file(COPY ${fff_SOURCE_DIR}/fff.h DESTINATION ${CMAKE_BINARY_DIR}/fff_include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/fff_include)

enable_testing()
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
