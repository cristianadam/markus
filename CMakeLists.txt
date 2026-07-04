cmake_minimum_required(VERSION 3.20)
project(markus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Profile build type support (for gprof)
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -pg")

# Add cmark as subdirectory for benchmark (EXCLUDE_FROM_ALL so it only builds when needed)
# Disable cmark's own tests since we use our own gtest-based test suite
set(BUILD_TESTING_SAVE ${BUILD_TESTING})
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(commonmark-spec cmark-build EXCLUDE_FROM_ALL)
set(BUILD_TESTING ${BUILD_TESTING_SAVE} CACHE BOOL "Build tests" FORCE)

# Markus header-only library
add_library(markus INTERFACE)
target_include_directories(markus INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Main executable
add_executable(main main.cc)
target_link_libraries(main PRIVATE markus)

# Tests
if(BUILD_TESTING)
  enable_testing()

  # Fetch googletest for the test executable
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
  )
  # For macOS ARM, suppress an upstream gtest warning
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(GTEST_HAS_THREAD_LIBRARY 0)
  endif()
  FetchContent_MakeAvailable(googletest)

  # Test executable using gtest to wrap the Python spec test runner
  add_executable(test_markus tests/test_markus.cc)
  target_link_libraries(test_markus PRIVATE markus GTest::gtest_main)
  set_target_properties(test_markus PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  )

  include(GoogleTest)
  gtest_discover_tests(test_markus
    TEST_PREFIX "markus."
    DISCOVERY_TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()

# Benchmark executable
add_executable(bench bench.cc)
target_link_libraries(bench PRIVATE markus cmark)
set_target_properties(bench PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Custom target to run benchmark
add_custom_target(run_bench
  COMMAND bench --iterations 100
  DEPENDS bench
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Running benchmark..."
)
