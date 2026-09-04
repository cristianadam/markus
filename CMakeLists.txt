cmake_minimum_required(VERSION 3.20)
project(markus LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Opt-in GFM feature tests (cmark-gfm GFM spec vs markus). Off by default so the
# standard CommonMark run stays green; enable to track GFM feature progress.
option(MARKUS_BUILD_GFM_TESTS "Build the opt-in cmark-gfm GFM feature tests" OFF)

# Fuzzing harness (see fuzz/). MARKUS_BUILD_FUZZER builds the standalone / AFL++
# driver (works with any compiler). MARKUS_BUILD_FUZZER_LIBFUZZER additionally
# builds the libFuzzer harness; that target uses the project's
# CMAKE_CXX_COMPILER, which must ship the fuzzer runtime (configure the tree
# with a fuzzer-capable clang, e.g. -DCMAKE_CXX_COMPILER=/path/to/clang++).
option(MARKUS_BUILD_FUZZER "Build the markus fuzzing harness (standalone / AFL driver)" OFF)
option(MARKUS_BUILD_FUZZER_LIBFUZZER "Build the libFuzzer fuzzing harness" OFF)

# Profile build type support (for gprof)
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -pg")

# Add cmark-gfm as subdirectory for benchmark (EXCLUDE_FROM_ALL so it only builds when needed)
# Only build the static core library; disable cmark-gfm's own tests since we use our own gtest suite
set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
set(CMARK_STATIC ON CACHE BOOL "" FORCE)
# cmark-gfm declares cmake_minimum_required(VERSION 3.5), which recent CMake
# rejects; allow it to configure.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
add_subdirectory(cmark-gfm cmark-gfm-build EXCLUDE_FROM_ALL)

# Add md4c as subdirectory for benchmark (EXCLUDE_FROM_ALL so it only builds when needed)
# Disable md4c's own executables since we only need the libraries
set(BUILD_MD2HTML_EXECUTABLE_SAVE ${BUILD_MD2HTML_EXECUTABLE})
set(BUILD_MD2HTML_EXECUTABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(md4c md4c-build EXCLUDE_FROM_ALL)
set(BUILD_MD2HTML_EXECUTABLE ${BUILD_MD2HTML_EXECUTABLE_SAVE} CACHE BOOL "" FORCE)

# Markus header-only library
add_library(markus INTERFACE)
target_include_directories(markus INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Main executable
add_executable(main main.cc)
target_link_libraries(main PRIVATE markus)

# Fuzzing harness (standalone / AFL driver, plus optional libFuzzer target)
if(MARKUS_BUILD_FUZZER OR MARKUS_BUILD_FUZZER_LIBFUZZER)
  add_subdirectory(fuzz)
endif()

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

  # Opt-in GFM feature tests: run cmark-gfm's GFM spec extension sections against
  # markus. Excluded from the default CommonMark run (see MARKUS_BUILD_GFM_TESTS).
  if(MARKUS_BUILD_GFM_TESTS)
    add_executable(test_gfm tests/test_gfm.cc)
    target_link_libraries(test_gfm PRIVATE markus GTest::gtest_main)
    set_target_properties(test_gfm PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    gtest_discover_tests(test_gfm
      TEST_PREFIX "markus.gfm."
      DISCOVERY_TIMEOUT 30
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
  endif()
endif()

# Google Benchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.1
)
FetchContent_MakeAvailable(googlebenchmark)

# Benchmark executable. libcmark-gfm-extensions_static provides the GFM
# extensions (table, strikethrough, autolink, ...) used by the autolink
# benchmark; the extensions include dir supplies cmark-gfm-core-extensions.h.
add_executable(bench bench.cc)
# The extensions archive must precede the core archive: GNU ld scans static
# archives in a single left-to-right pass, and table.o (extensions) references
# cmark_arena_push/pop (core/arena.o), so the core library must still be
# unscanned when table.o is pulled in. Matches cmark-gfm's own link order.
target_link_libraries(bench PRIVATE markus libcmark-gfm-extensions_static libcmark-gfm_static md4c md4c-html benchmark::benchmark)
target_include_directories(bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cmark-gfm/src ${CMAKE_BINARY_DIR}/cmark-gfm-build/src ${CMAKE_CURRENT_SOURCE_DIR}/cmark-gfm/extensions)
set_target_properties(bench PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Custom target to run benchmark
add_custom_target(run_bench
  COMMAND bench --rounds 2 --benchmark_min_time=0.5
  DEPENDS bench
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Running benchmark..."
)
