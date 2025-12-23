Single header markdown file implemented in hyde.h using C++20 with no 
external dependencies, implemented in GoogleStyle, adhering to CommonMark spec,
and tested via `run_tests.sh` to run all tests or by 
`python get_test_output [TEST_NUMBER]` to get output for specific tests.
The file `main.cc` runs the hyde.h library. The main executable to test input 
Markdown (via stdin) and output HTML (via stdout) is built via 
`bazel build main`.