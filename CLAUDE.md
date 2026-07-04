Single header markdown file implemented in markus.h using C++20 with no 
external dependencies, implemented in GoogleStyle, adhering to CommonMark spec,
and tested via `run_tests.sh` to run all tests or by the following that takes
in one or more specific test numbers that can be run.

`run_tests.sh [TEST_NUMBER] ...`

The file `main.cc` runs the markus.h library. The main executable to test input 
Markdown (via stdin) and output HTML (via stdout) is built via CMake.

## Build system: CMake

Configure and build:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build
```

Run all 655 CommonMark spec tests:
```bash
ctest --test-dir build
```

Run specific test numbers (e.g. tests 1, 2, 3):
```bash
ctest --test-dir build -R "Example_(1|2|3)$"
```

Run the benchmark (compares markus vs cmark):
```bash
./run_bench.sh [--iterations N] [SAMPLES_DIR]
# or via CMake target:
cmake --build build --target run_bench
```

Build types: `Release` (default), `Debug`, `Profile` (adds `-pg` for gprof).

To build without tests (faster, e.g. for CI release builds):
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build
```

## Key files
- `markus.h` — single-header C++20 Markdown-to-HTML library (lookup tables, inline optimizations)
- `main.cc` — CLI entry point: reads Markdown from stdin, outputs HTML to stdout (`--ast` flag for AST debug output)
- `bench.cc` — benchmark comparing markus vs cmark performance
- `tests/test_markus.cc` — gtest-based test suite wrapping Python spec_tests.py (655 examples)
- `CMakeLists.txt` — top-level CMake build configuration
- `run_tests.sh` — wrapper that builds and runs ctest (supports specific test numbers)
- `run_bench.sh` — wrapper that builds and runs the benchmark binary
