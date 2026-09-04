# Fuzzing harness for markus.
#
# Two targets can be built:
#   fuzz_markus            - libFuzzer harness. Requires a clang that ships the
#                            libFuzzer runtime (a full LLVM build; NOT the
#                            default Apple clang). Gate with
#                            -DMARKUS_BUILD_FUZZER_LIBFUZZER=ON and configure
#                            the whole tree with such a compiler
#                            (-DCMAKE_CXX_COMPILER=/path/to/clang++).
#   fuzz_markus_standalone - standalone / AFL++ driver (built with any
#                            compiler). Run it directly to replay inputs or as
#                            the AFL++ target (`./fuzz_markus_standalone @@`).
#
# Both use the project's CMAKE_CXX_COMPILER. For the libFuzzer target that must
# be a compiler shipping the fuzzer runtime, so configure the whole tree with
# it (e.g. -DCMAKE_CXX_COMPILER=/path/to/clang++).
#
# Both are instrumented with AddressSanitizer (the standalone build also adds
# UndefinedBehaviorSanitizer) so memory bugs and overflows are caught.

# Fuzz builds want -O1 (a good coverage/speed balance) plus -g.
set(MARKUS_FUZZ_COMPILE_OPTS -O1 -g -fsanitize=address,undefined)
set(MARKUS_FUZZ_LINK_OPTS -fsanitize=address,undefined)

# --- Standalone / AFL++ driver (always buildable) ---------------------------
add_executable(fuzz_markus_standalone fuzz_markus.cc)
target_link_libraries(fuzz_markus_standalone PRIVATE markus)
target_compile_definitions(fuzz_markus_standalone PRIVATE MARKUS_FUZZ_STANDALONE)
target_compile_options(fuzz_markus_standalone PRIVATE ${MARKUS_FUZZ_COMPILE_OPTS})
target_link_options(fuzz_markus_standalone PRIVATE ${MARKUS_FUZZ_LINK_OPTS})
set_target_properties(fuzz_markus_standalone PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# --- libFuzzer harness (optional) -------------------------------------------
# Uses the project compiler; configure the tree with a fuzzer-capable
# CMAKE_CXX_COMPILER (see header comment). The fuzzer runtime provides main(),
# so link -fsanitize=fuzzer (not fuzzer-no-link).
if(MARKUS_BUILD_FUZZER_LIBFUZZER)
  add_executable(fuzz_markus fuzz_markus.cc)
  target_link_libraries(fuzz_markus PRIVATE markus)
  target_compile_options(fuzz_markus PRIVATE -O1 -g -fsanitize=fuzzer,address)
  target_link_options(fuzz_markus PRIVATE -fsanitize=fuzzer,address)
  set_target_properties(fuzz_markus PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif()
