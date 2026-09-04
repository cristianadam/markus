Single header markdown file implemented in markus.h using C++20 with no 
external dependencies, implemented in GoogleStyle, adhering to CommonMark spec
plus opt-in GFM extensions (table, autolink, strikethrough, tasklist, tagfilter),
a streaming API (`StreamingMarkdownParser`) for progressive rendering of
incrementally-arriving Markdown,
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

Run the benchmark (compares markus vs cmark-gfm vs md4c; cmark-gfm runs in plain CommonMark mode):
```bash
./run_bench.sh [--iterations N] [SAMPLES_DIR]
# or via CMake target:
cmake --build build --target run_bench
```

Opt-in GFM feature tests (run cmark-gfm's GFM spec extension sections against
markus). Markus implements the GFM extensions (table, autolink, strikethrough,
tasklist, tagfilter); these tests exercise them against cmark-gfm's reference
output and are excluded from the default CommonMark run:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMARKUS_BUILD_GFM_TESTS=ON
cmake --build build --target test_gfm
ctest --test-dir build -R 'markus\.gfm\.' --output-on-failure
```

Build types: `Release` (default), `Debug`, `Profile` (adds `-pg` for gprof).

To build without tests (faster, e.g. for CI release builds):
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build
```

## Fuzzing

`fuzz/` holds the fuzzing harness (modeled on cmark-gfm's `fuzz/`): a single
`fuzz_markus.cc` drives a quadratic amplifier (config prefix + repeated segment)
through both the batch path (`Parse`/`RenderHtml`/`DebugAst`) and the
`StreamingMarkdownParser`. Two targets can be built (see `-DMARKUS_BUILD_FUZZER`):

- `fuzz_markus_standalone` — standalone / AFL++ driver, built with any compiler
  (`-DMARKUS_BUILD_FUZZER=ON`). Replays a file or directory:
  `./build/fuzz_markus_standalone fuzz/corpus`. This is the recommended local
  path — run it under AFL++ with `./fuzz/run_fuzz.sh afl` (dumb mode, or build
  the target with `afl-clang-fast` for edge-guided fuzzing).
- `fuzz_markus` — libFuzzer harness, best in CI/Linux where the per-unit
  `-timeout` works. Configure the whole tree with a fuzzer-capable compiler
  (the default Apple clang lacks the libFuzzer runtime):
  ```bash
  cmake -S . -B build-fuzz -DBUILD_TESTING=OFF \
        -DCMAKE_CXX_COMPILER=/path/to/fuzzer-capable/clang++ \
        -DMARKUS_BUILD_FUZZER=ON -DMARKUS_BUILD_FUZZER_LIBFUZZER=ON
  cmake --build build-fuzz --target fuzz_markus
  BUILDDIR=build-fuzz ./fuzz/run_fuzz.sh libfuzzer
  ```

`fuzz/corpus/` are the seed inputs and `fuzz/fuzzing_dictionary` is the libFuzzer
dictionary. `fuzz/run_fuzz.sh` wraps the build-and-run commands
(`replay` | `libfuzzer` | `afl`).

## Key files
- `markus.h` — single-header C++20 Markdown-to-HTML library (lookup tables, inline optimizations; includes `StreamingMarkdownParser` for incremental/progressive rendering)
- `main.cc` — CLI entry point: reads Markdown from stdin, outputs HTML to stdout (`--ast` flag for AST debug output; `-e <ext>` enables a GFM extension: table, autolink, strikethrough, tasklist, tagfilter; `--unsafe` accepts raw HTML; `--stream` emits HTML as blocks complete, reading stdin in chunks)
- `bench.cc` — benchmark comparing markus vs cmark-gfm vs md4c performance (plain CommonMark suite plus per-extension GFM benchmarks)
- `tests/test_markus.cc` — gtest-based test suite wrapping the CommonMark spec_tests.py (655 examples, spec from the `commonmark-spec` submodule) plus the `StreamingMarkdownParser` test suite
- `tests/test_gfm.cc` — opt-in GFM feature tests (cmark-gfm GFM spec vs markus; `-DMARKUS_BUILD_GFM_TESTS=ON`)
- `fuzz/fuzz_markus.cc` — fuzzing harness (quadratic amplifier; exercises batch + streaming paths); builds as the libFuzzer target `fuzz_markus` and as the standalone/AFL driver `fuzz_markus_standalone`

Submodules: `commonmark-spec` (cmark) supplies the CommonMark spec for the compliance suite;
`cmark-gfm` supplies the benchmark library and the GFM spec for feature tests; `md4c` is a
benchmark comparison target.
- `CMakeLists.txt` — top-level CMake build configuration
- `run_tests.sh` — wrapper that builds and runs ctest (supports specific test numbers)
- `run_bench.sh` — wrapper that builds and runs the benchmark binary
